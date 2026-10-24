from pathlib import Path
import tempfile
import importlib.util
import unittest

TOOL = Path(__file__).resolve().parents[2] / "tools/build-articles.py"

class ReaderBuildTests(unittest.TestCase):
    def test_cpp_lambda_in_code_is_not_rewritten_as_a_markdown_link(self):
        code = '```cpp\ninvoke([mat](auto& c) { c.setMaterial(mat); });\n```\n`[this](int)`'
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.assertEqual(self.module().prepare_body(code, root, root), code)

    def test_stage_two_chapter_and_offline_figures(self):
        articles = self.module().load_articles()
        self.assertEqual(len(articles), 6)
        chapter = next(a for a in articles if a["id"] == "05-stage-two-lighting")
        self.assertIn("data:image/png;base64,", chapter["body"])
        self.assertGreaterEqual(chapter["body"].count("data:image/svg+xml;base64,"), 2)
        import xml.etree.ElementTree as ET
        root = TOOL.parents[1]
        for name in ("stage-two-lighting.svg", "stage-two-fbo.svg"):
            ET.parse(root / "articles/assets" / name)

    def test_studio_contract_chapter_is_offline(self):
        chapter = next(a for a in self.module().load_articles() if a["id"] == "06-studio-pipeline")
        self.assertGreaterEqual(chapter["body"].count("data:image/png;base64,"), 2)
        self.assertIn("data:image/svg+xml;base64,", chapter["body"])
        self.assertIn("../api/openapi.json", chapter["body"])
        self.assertIn("PipelineState::restore", chapter["body"])

    def module(self):
        spec = importlib.util.spec_from_file_location("reader", TOOL)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

    def test_json_cannot_close_inline_script(self):
        # A shader/source example may legitimately contain script-like text.
        # Serializing it must not terminate the reader's data script.
        encoded = self.module().script_json({"body": "</script><script>alert(1)</script>"})
        self.assertNotIn("</script", encoded.lower())
        import json
        self.assertEqual(json.loads(encoded)["body"], "</script><script>alert(1)</script>")

    def test_images_become_offline_data_and_source_links_stay_relative(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); article=root/"articles"/"chapter"; article.mkdir(parents=True)
            (article/"cover.svg").write_text('<svg xmlns="http://www.w3.org/2000/svg"/>')
            (root/"native").mkdir(); (root/"native"/"scene.cpp").write_text("int main(){}")
            body=self.module().prepare_body("![cover](cover.svg) [source](../../native/scene.cpp)",article,root/"articles")
            self.assertIn("data:image/svg+xml;base64,",body)
            self.assertIn("../native/scene.cpp",body)

if __name__ == "__main__": unittest.main()
