#!/usr/bin/python

import json
from urllib.request import urlopen

import jsonschema


def validate(instance, schema_filename=None, follow_links=False, versions_schema_filename=None, *,
             instance_filename=None, verbose=False):
    if not schema_filename:
        schema_filename = os.path.join(os.path.dirname(__file__), "../schema/packs.schema.json")
    if not versions_schema_filename:
        versions_schema_filename = os.path.join(os.path.dirname(__file__), "../schema/versions.schema.json")
    with open(schema_filename, "r") as schema_file:
        if verbose:
            print(f"Validating {instance_filename or 'json'}")
        jsonschema.validate(instance=instance, schema=json.load(schema_file))
        if follow_links:
            with open(versions_schema_filename, "r") as versions_schema_file:
                versions_schema = json.load(versions_schema_file)
            for uid, pack in instance.items():
                try:
                    versions_url = pack["versions_url"]
                    if verbose:
                        print(f"Validating {uid} versions: {versions_url}")
                    versions_data = json.load(urlopen(versions_url))
                    jsonschema.validate(instance=versions_data, schema=versions_schema)
                except Exception as e:
                    raise ValueError(f"Error validating {versions_url} for {uid}") from e

    return True


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
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()
    validate(json.load(args.infile), args.schema, args.follow_links, args.versions_schema,
             instance_filename=args.infile.name,
             verbose=args.verbose, )
    exit(0)
