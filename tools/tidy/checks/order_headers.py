from abstract_check import Check
from pathlib import Path
from typing import Optional
import re

__all__ = ["OrderHeadersCheck"]

INCLUDE_REGEX = re.compile(r"(?:<|\")(.+)(?:>|\")")

class OrderHeadersCheck(Check):
    def check_ordering(self, path: Path) -> Optional[str]:
        file = path.read_text()
        includes = [line for line in file.splitlines() if line.startswith("#include ")]

        own_include = []
        same_module = []
        other_modules = []
        stdlib = []
        common = []

        is_header = path.suffix in (".hh", ".h")
        for include in includes:
            include = include.removeprefix("#include ")

            match = INCLUDE_REGEX.match(include)
            assert match is not None
            file = match.group(1)

            if Path(file).name == path.name:
                if is_header:
                    return "Include of self"
                elif len(own_include) != 0:
                    return "Multiple includes of own header"
                else:
                    own_include.append(file)





        raise SystemExit

    def run(self, files: list[Path]) -> Optional[Path]:
        for header in files:
            if header.suffix in (".cc", ".hh", ".c", ".h"):
                self.check_ordering(header)
