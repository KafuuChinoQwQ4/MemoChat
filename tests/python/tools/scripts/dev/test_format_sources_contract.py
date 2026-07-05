import importlib.util
import unittest

from tests.python.support.paths import repo_root

REPO_ROOT = repo_root()
FORMAT_SOURCES = REPO_ROOT / "tools/scripts/dev/format_sources.py"


def load_format_sources_module():
    spec = importlib.util.spec_from_file_location("memochat_format_sources", FORMAT_SOURCES)
    if spec is None or spec.loader is None:
        raise AssertionError(f"Could not load {FORMAT_SOURCES}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class FormatSourcesContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.format_sources = load_format_sources_module()

    def test_repo_root_resolves_to_project_root_from_nested_script_path(self):
        self.assertEqual(self.format_sources.repo_root(), REPO_ROOT)

    def test_cpp_source_scan_uses_project_source_roots(self):
        files = self.format_sources.iter_source_files(REPO_ROOT, self.format_sources.CPP_SUFFIXES)
        paths = {path.as_posix() for path in files}

        self.assertIn("apps/server/core/ChatServer/app/ChatServer.cpp", paths)
        self.assertIn("apps/server/core/common/json/GlazeCompat.hpp", paths)

    def test_python_source_scan_uses_project_source_roots(self):
        files = self.format_sources.iter_source_files(REPO_ROOT, self.format_sources.PYTHON_SUFFIXES)
        paths = {path.as_posix() for path in files}

        self.assertIn("tools/scripts/dev/format_sources.py", paths)
        self.assertIn("tests/python/tools/scripts/dev/test_format_sources_contract.py", paths)


if __name__ == "__main__":
    unittest.main()
