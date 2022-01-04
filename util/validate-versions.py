#!/usr/bin/python

import json
import jsonschema

def validate(instance, schemafilename=None):
    if not schemafilename:
        schemafilename = os.path.join(os.path.dirname(__file__), '../schema/versions.schema.json')
    with open(schemafilename, 'r') as schemafile:
        jsonschema.validate(instance=instance, schema=json.load(schemafile))
    return True

if __name__ == '__main__':
    import argparse
    import os
    parser = argparse.ArgumentParser(description='Validate versions.json')
    parser.add_argument('infile', type=argparse.FileType('r'))
    parser.add_argument('--schema', type=str)
    args = parser.parse_args()
    validate(json.load(args.infile), args.schema)
    exit(0)
