#include <iostream>
#include <luma/studio/ApiContract.hpp>
#include <luma/studio/StudioDocument.hpp>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
using namespace luma;
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QFile file(QDir(QFileInfo(__FILE__).absolutePath()).filePath("../../api/examples.json"));
    if (!file.open(QIODevice::ReadOnly)) return 1;
    const auto examples = QJsonDocument::fromJson(file.readAll()).object();
    ApiContract contract;
    for (auto it = examples.begin(); it != examples.end(); ++it) contract.validate(it.key(), it.value());
    int failures = 0;
    auto rejected = [&](const QJsonObject& value) {
        try { contract.validate("ai.RunRequest", value); ++failures; }
        catch (const std::exception&) { std::cout << "PASS invalid request rejected\n"; }
    };
    auto request = examples["ai.RunRequest"].toObject();
    request["unexpected"] = true; rejected(request);
    request.remove("unexpected"); request["capability"] = "vlm"; rejected(request);
    request = examples["ai.RunRequest"].toObject(); request["project_id"] = "bad"; rejected(request);
    const auto operation = contract.operation("createRun");
    if (operation.method != "POST" || operation.path != "/v1/runs") ++failures;
    StudioDocument doc;
    const auto emoji = QString::fromUtf8("😀").repeated(3000);
    const auto context = doc.requestMessages(emoji, false);
    if (context.last().toObject()["text"] != emoji) ++failures;
    doc.messages.append(QJsonObject{{"role", "assistant"}, {"text", emoji.repeated(2)}});
    if (doc.requestMessages("继续", false).first().toObject()["text"].toString().toUcs4().size() != 4000) ++failures;
    doc.pipeline.start(); doc.pipeline.prepared(); doc.pipeline.analyzed(); doc.pipeline.cancel();
    const auto saved = doc.toJson();
    const bool preserved = StudioDocument::fromJson(saved).pipelineJson() == doc.pipelineJson();
    failures += !preserved;
    std::cout << (preserved ? "PASS" : "FAIL") << " cancelled review survives project restore\n";
    auto invalid = saved;
    auto plan = invalid["pipeline"].toObject(); auto nodes = plan["nodes"].toArray();
    auto first = nodes[0].toObject(); first["status"] = "pending"; nodes[0] = first;
    plan["nodes"] = nodes; invalid["pipeline"] = plan;
    try { StudioDocument::fromJson(invalid); ++failures; std::cout << "FAIL impossible pipeline accepted\n"; }
    catch (const std::exception&) { std::cout << "PASS impossible pipeline rejected\n"; }
    doc.pipeline.reset(); doc.pipeline.start(); doc.pipeline.prepared();
    const auto interrupted = StudioDocument::fromJson(doc.toJson());
    if (interrupted.pipeline.nodes()[1] != PipelineNodeState::Failed) ++failures;
    if (argc > 1) {
        QFile output(QString::fromLocal8Bit(argv[1]));
        if (!output.open(QIODevice::WriteOnly)) return 1;
        auto client = examples; client["studio.Document"] = saved;
        output.write(QJsonDocument(client).toJson());
    }
    std::cout << (failures ? "FAIL" : "PASS") << " contract examples and operation routing\n";
    return failures ? 1 : 0;
}
