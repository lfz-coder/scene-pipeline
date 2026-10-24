"""Validate the real OpenAPI and the same DTOs emitted by the Qt client."""
import copy
import json
import os
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ContractTests(unittest.TestCase):
    def test_contract_and_examples(self):
        self.assertTrue((ROOT / 'api/openapi.json').exists(), 'OpenAPI contract is required')
        from openapi_spec_validator import validate
        from jsonschema import Draft202012Validator, FormatChecker, ValidationError
        from referencing import Registry, Resource
        spec = json.loads((ROOT / 'api/openapi.json').read_text())
        validate(spec, base_uri=(ROOT / 'api/openapi.json').as_uri())
        registry = Registry()
        for path in (ROOT / 'api/schemas').glob('*.json'):
            schema = json.loads(path.read_text())
            registry = registry.with_resource(path.as_uri(), Resource.from_contents(schema))
        fixture = json.loads((ROOT / 'api/examples.json').read_text())
        client_path = os.environ.get('LUMA_CLIENT_FIXTURES')
        if client_path:
            fixture.update(json.loads(Path(client_path).read_text()))
            self.assertIn('studio.Document', fixture)
        for name, value in fixture.items():
            schema = {'$ref': (ROOT / f'api/schemas/{name.split(".")[0]}.json').as_uri()
                             + '#/$defs/' + name.split('.')[1]}
            with self.subTest(schema=name):
                Draft202012Validator(schema, registry=registry, format_checker=FormatChecker()).validate(value)
        request = copy.deepcopy(fixture['ai.RunRequest'])
        request['capability'] = 'vlm'
        with self.assertRaises(ValidationError):
            Draft202012Validator({'$ref': (ROOT / 'api/schemas/ai.json').as_uri()
                                 + '#/$defs/RunRequest'}, registry=registry).validate(request)
        request = copy.deepcopy(fixture['ai.RunRequest'])
        request['unexpected'] = True
        with self.assertRaises(ValidationError):
            Draft202012Validator({'$ref': (ROOT / 'api/schemas/ai.json').as_uri()
                                 + '#/$defs/RunRequest'}, registry=registry).validate(request)
        for field, value in [('project_id', 'invalid-uuid'), ('messages', []), ('images', [{
                'mime_type': 'video/mp4', 'data_base64': 'AAAA'}])]:
            request = copy.deepcopy(fixture['ai.RunRequest'])
            request[field] = value
            with self.subTest(invalid=field), self.assertRaises(ValidationError):
                Draft202012Validator({'$ref': (ROOT / 'api/schemas/ai.json').as_uri()
                                     + '#/$defs/RunRequest'}, registry=registry).validate(request)


if __name__ == '__main__':
    unittest.main()
