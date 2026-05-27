import json
import os
import hashlib
from datetime import datetime, timezone

version = os.environ.get('SEMANTIC_RELEASE_NEXT_RELEASE_VERSION', 'unknown')
git_tag = os.environ.get('SEMANTIC_RELEASE_NEXT_RELEASE_GIT_TAG', version)
repo = os.environ.get('GITHUB_REPOSITORY', '')
timestamp = datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')

files_list = ['firmware.bin', 'littlefs.bin', 'bootloader.bin', 'partitions.bin']
files_info = {}

for name in files_list:
    path = os.path.join('dist', name)
    if os.path.exists(path):
        with open(path, 'rb') as f:
            data = f.read()
        md5 = hashlib.md5(data).hexdigest()
        size = len(data)

        url = f'https://github.com/{repo}/releases/download/{git_tag}/{name}' if repo else name
        files_info[name] = {'url': url, 'size': size, 'md5': md5}

latest = {'version': version, 'timestamp': timestamp, 'files': files_info}
with open('dist/latest.json', 'w') as f:
    json.dump(latest, f, indent=2)

print(f'Generated dist/latest.json for version {version}')
