#!/usr/bin/python

import json
from enum import Enum
from typing import Any, Dict, Optional
from urllib.request import urlopen

import jsonschema


class FollowMode(Enum):
    NONE = 0
    FIRST = 1
    ALL = 2


def validate(
        instance: Any,
        schema_filename: Optional[str] = None,
        follow_links: bool = False,
        versions_schema_filename: Optional[str] = None,
        *,
        follow_recursive: FollowMode = FollowMode.NONE,
        instance_filename: Optional[str] = None,
        verbose: bool = False
) -> None:
    if not schema_filename:
        schema_filename = os.path.join(os.path.dirname(__file__), "../schema/packs.schema.json")
    if not versions_schema_filename:
        versions_schema_filename = os.path.join(os.path.dirname(__file__), "../schema/versions.schema.json")
    with open(schema_filename, "r") as schema_file:
        if verbose:
            print(f"Validating {instance_filename or 'json'}")
        jsonschema.validate(instance=instance, schema=json.load(schema_file))
        if follow_links:
            import importlib

            versions_mod = importlib.import_module("validate-versions")
            with open(versions_schema_filename, "r") as versions_schema_file:
                versions_schema = json.load(versions_schema_file)
            for uid, pack in instance.items():
                try:
                    versions_url = pack["versions_url"]
                    if verbose:
                        print(f"Validating {uid} versions: {versions_url}")
                    versions_data = json.load(urlopen(versions_url))
                    jsonschema.validate(instance=versions_data, schema=versions_schema)
                    if follow_recursive != FollowMode.NONE:
                        for entry in versions_data["versions"]:
                            if (versions_mod.validate_version_entry(entry, uid, verbose=verbose)
                                    and follow_recursive == FollowMode.FIRST):
                                break
                except Exception as e:
                    raise ValueError(f"Error validating {versions_url} for {uid}") from e


if __name__ == "__main__":
    import argparse
    import os

    parser = argparse.ArgumentParser(description="Validate packs.json")
    parser.add_argument("infile", type=argparse.FileType("r"))
    parser.add_argument("--schema", type=str,
                        help="schema to use for validation")
    parser.add_argument("--versions-schema", type=str, dest="versions_schema",
                        help="schema to use for version.json files if --follow-links is used")
    parser.add_argument("--follow-links", action="store_true", dest="follow_links",
                        help="follow links and validate linked files.")
    parser.add_argument("--recursive", action="store_true", dest="recursive",
                        help="when combined with --follow-links, follows links in linked files.")
    parser.add_argument("--only-first", action="store_true", dest="only_first",
                        help="when combined with --recursive, only follows first entry of a list.")
    parser.add_argument("--diff", type=argparse.FileType("r"), dest="diff",
                        help="Only check changes from file passed for diff.")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()
    recursive_mode = FollowMode.NONE if not args.recursive else FollowMode.FIRST if args.only_first else FollowMode.ALL
    data: Dict[str, Any] = json.load(args.infile)
    if args.diff:
        orig: Dict[str, Any] = json.load(args.diff)
        data = {
            k: v
            for k, v in data.items()
            if k not in orig or v != orig[k]
        }
    validate(
        data,
        args.schema,
        args.follow_links,
        args.versions_schema,
        follow_recursive=recursive_mode,
        instance_filename=args.infile.name,
        verbose=args.verbose,
    )
    exit(0)
