"""Check local Markdown file targets, including new untracked documentation."""
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote, urlsplit

root = Path(__file__).resolve().parents[1]
paths = subprocess.check_output(['git', 'ls-files', '--cached', '--others', '--exclude-standard',
                                 '-z', '--', '*.md'], cwd=root).decode().split('\0')
errors = []
checked = 0
for name in filter(None, paths):
    path = root / name
    for match in re.finditer(r'!?\[[^\]]*\]\(([^)\s]+)\)', path.read_text()):
        target = urlsplit(match.group(1))
        if target.scheme or target.netloc or not target.path:
            continue
        checked += 1
        if not (path.parent / unquote(target.path)).exists():
            errors.append(f'{name}: {match.group(1)}')
if errors:
    raise SystemExit('\n'.join(errors))
print(f'PASS: {checked} local Markdown file targets (external URLs/anchors are not checked)')
