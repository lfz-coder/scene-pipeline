#include <luma/studio/ApiContract.hpp>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <cmath>
#include <stdexcept>
static void initApiResources() { Q_INIT_RESOURCE(luma_api); }
namespace luma {
namespace {
[[noreturn]] void invalid(const QString& path, const QString& reason) {
    throw std::invalid_argument(("契约校验失败：" + path + " · " + reason).toStdString());
}
QJsonObject read(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) invalid(path, "无法读取规范");
    QJsonParseError error;
    const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) invalid(path, "规范格式错误");
    return doc.object();
}
}
ApiContract::ApiContract() {
    static const bool initialized = [] { initApiResources(); return true; }();
    (void)initialized;
    spec_ = read(":/luma-api/openapi.json");
    for (const auto* file : {"common", "ai", "pipeline", "timeline", "studio"})
        schemas_.insert(QString(file) + ".json", read(QString(":/luma-api/schemas/%1.json").arg(file)));
}
ApiOperation ApiContract::operation(const QString& id) const {
    const auto paths = spec_["paths"].toObject();
    for (auto it = paths.begin(); it != paths.end(); ++it) {
        const auto methods = it.value().toObject();
        for (auto method = methods.begin(); method != methods.end(); ++method)
            if (method.value().toObject()["operationId"] == id)
                return {method.key().toUpper().toLatin1(), it.key()};
    }
    invalid(id, "不存在的 operationId");
}
QJsonObject ApiContract::resolve(const QString& reference) const {
    const auto file = reference.section('#', 0, 0).section('/', -1);
    const auto name = reference.section("#/$defs/", 1);
    const auto schema = schemas_.value(file).value("$defs").toObject().value(name);
    if (!schema.isObject()) invalid(reference, "无法解析引用");
    return schema.toObject();
}
void ApiContract::validate(const QString& name, const QJsonValue& value) const {
    check(resolve(name.section('.', 0, 0) + ".json#/$defs/" + name.section('.', 1)), value, name);
}
void ApiContract::check(const QJsonObject& schema, const QJsonValue& value, const QString& path) const {
    static const QSet<QString> known{"$ref", "$schema", "$defs", "type", "properties", "required",
        "additionalProperties", "items", "minItems", "maxItems", "minLength", "maxLength",
        "minimum", "maximum", "enum", "const", "pattern", "format", "if", "then"};
    for (auto it = schema.begin(); it != schema.end(); ++it)
        if (!known.contains(it.key())) invalid(path, "未实现的校验关键字 " + it.key());
    if (schema.contains("$ref")) check(resolve(schema["$ref"].toString()), value, path);
    const auto type = schema["type"].toString();
    bool valid = type.isEmpty() || (type == "object" && value.isObject()) ||
        (type == "array" && value.isArray()) || (type == "string" && value.isString()) ||
        (type == "boolean" && value.isBool()) || (type == "integer" && value.isDouble() &&
          std::isfinite(value.toDouble()) && std::floor(value.toDouble()) == value.toDouble());
    if (!valid) invalid(path, "类型应为 " + type);
    if (schema.contains("const") && value != schema["const"]) invalid(path, "固定值不匹配");
    if (schema.contains("enum") && !schema["enum"].toArray().contains(value)) invalid(path, "枚举不匹配");
    auto bounds = [&](double n, const char* lo, const char* hi) {
        if ((schema.contains(lo) && n < schema[lo].toDouble()) ||
            (schema.contains(hi) && n > schema[hi].toDouble())) invalid(path, "超出允许范围");
    };
    if (value.isDouble()) bounds(value.toDouble(), "minimum", "maximum");
    if (value.isString()) {
        const auto text = value.toString();
        bounds(text.toUcs4().size(), "minLength", "maxLength");
        if (schema.contains("pattern") && !QRegularExpression(schema["pattern"].toString()).match(text).hasMatch())
            invalid(path, "字符串格式不匹配");
        if (schema["format"] == "uri" && (!QUrl(text).isValid() || QUrl(text).isRelative()))
            invalid(path, "须为绝对 URI");
    }
    if (value.isArray()) {
        const auto values = value.toArray();
        bounds(values.size(), "minItems", "maxItems");
        if (schema.contains("items"))
            for (int i = 0; i < values.size(); ++i)
                check(schema["items"].toObject(), values[i], path + QString("[%1]").arg(i));
    }
    if (value.isObject()) {
        const auto object = value.toObject(), properties = schema["properties"].toObject();
        for (const auto& key : schema["required"].toArray())
            if (!object.contains(key.toString())) invalid(path, "缺少 " + key.toString());
        for (auto it = object.begin(); it != object.end(); ++it) {
            if (properties.contains(it.key())) check(properties[it.key()].toObject(), it.value(), path + "." + it.key());
            else if (schema["additionalProperties"] == false) invalid(path, "未知字段 " + it.key());
        }
    }
    if (schema.contains("if")) {
        bool matches = true;
        try { check(schema["if"].toObject(), value, path); }
        catch (const std::invalid_argument&) { matches = false; }
        if (matches) check(schema["then"].toObject(), value, path);
    }
}
} // namespace luma
