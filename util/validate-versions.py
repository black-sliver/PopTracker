#!/usr/bin/python

import json
import re
from enum import Enum
from hashlib import sha256
from tempfile import TemporaryFile
from typing import Any, Dict, IO, Optional, Union
from zipfile import ZipFile

import jsonschema


class FollowMode(Enum):
    NONE = 0
    FIRST = 1
    ALL = 2


def load_jsonc(f: Union[IO[bytes], IO[str]]) -> Any:
    comment_regex = re.compile(r"(\".*?\"|\'.*?\')|(/\*.*?\*/|//[^\r\n]*$)", re.MULTILINE | re.DOTALL)
    trailing_regex = re.compile(r"(\".*?\"|\'.*?\')|,(\s*[\]\}])", re.MULTILINE | re.DOTALL)

    data = f.read()
    if isinstance(data, bytes):
        data = data.decode("utf-8-sig")
    data = comment_regex.sub(lambda match: "" if match.group(2) is not None else match.group(1), data)
    data = trailing_regex.sub(lambda match: match.group(2) if match.group(2) is not None else match.group(1), data)
    return json.loads(data)


def validate_version_entry(entry: Dict[str, Any], uid: Optional[str], *, verbose: bool = False) -> bool:
    """Validate (links in) entry of versions.json. Returns False if the entry is uncheckable (no URL)."""
    import requests

    url: Optional[str] = entry.get("download_url", None)
    if url is None:
        return False
    if verbose:
        print(f"Downloading {url} ...")
    with TemporaryFile() as f:
        md = sha256()
        with requests.get(url, stream=True, allow_redirects=True) as r:
            r.raise_for_status()
            for chunk in r.iter_content(8192):
                md.update(chunk)
                f.write(chunk)
        if entry["sha256"].lower() != md.hexdigest():
            raise ValueError(f"Expected sha256 hash of {entry['sha256'].lower()} but got {md.hexdigest()} for {url}")
        f.seek(0)
        zf = ZipFile(f)
        for zi in zf.infolist():
            fn = zi.filename
            if fn == "manifest.json" or fn.endswith("/manifest.json") or fn.endswith("\\manifest.json"):
                manifest = load_jsonc(zf.open(zi))
                if not isinstance(manifest, dict):
                    raise ValueError(f"Invalid manifest: {fn} in {url}")
                manifest_uid = manifest.get("package_uid", None)
                if manifest_uid is None:
                    raise ValueError(f"package_uid missing from {fn} in {url}")
                if uid is not None and uid != manifest_uid:
                    raise ValueError(f"Wrong UID: '{manifest_uid}', expected '{uid}' in {url}")
                break
        else:
            raise ValueError(f"Did not find manifest.json in {url}")
    return True


def validate(
        instance: Any,
        schema_filename: Optional[str] = None,
        follow: FollowMode = FollowMode.NONE,
        uid: Optional[str] = None,
        *,
        verbose: bool = False,
) -> None:
    if not schema_filename:
        schema_filename = os.path.join(os.path.dirname(__file__), '../schema/versions.schema.json')
    with open(schema_filename, 'r') as schema_file:
        jsonschema.validate(instance=instance, schema=json.load(schema_file))
        if follow != FollowMode.NONE:
            for entry in instance["versions"]:
                if validate_version_entry(entry, uid, verbose=verbose) and follow == FollowMode.FIRST:
                    break


if __name__ == '__main__':
    import argparse
    import os
    parser = argparse.ArgumentParser(description='Validate versions.json')
    parser.add_argument('infile', type=argparse.FileType('r'))
    parser.add_argument('--schema', type=str)
    parser.add_argument("--follow-links", action="store_true", dest="follow_links",
                        help="follow links and validate linked files.")
    parser.add_argument("--only-first", action="store_true", dest="only_first",
                        help="when combined with --follow-links, only follows first entry of a list.")
    parser.add_argument("--uid", type=str,
                        help="when combined with --follow-links, check for this UID in pack manifest.")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()
    follow_mode = FollowMode.NONE if not args.follow_links else FollowMode.FIRST if args.only_first else FollowMode.ALL
    validate(json.load(args.infile), args.schema, follow_mode, args.uid, verbose=args.verbose)
    exit(0)
