#!/usr/bin/env python3
import base64
import sys
import typing as t
from datetime import datetime, timedelta
from pathlib import Path


def extract_from_comment(comment: str, key: str) -> t.Optional[str]:
    key += " "
    p = comment.find(key)
    if p == -1:
        return None
    if comment.find(key, p + len(key)) != -1:
        raise ValueError(f"Multiple {key}in comment")
    return comment[p + len(key) :].split(" ", 1)[0]  # value is only the first word after key


def get_hex_pub_key(comment: str) -> t.Optional[str]:
    return extract_from_comment(comment, "minisign public key")


def get_alias(comment: str) -> t.Optional[str]:
    return extract_from_comment(comment, "alias")


def get_not_before(comment: str) -> t.Optional[datetime]:
    s = extract_from_comment(comment, "not before")
    if s is None:
        return None
    return datetime.fromtimestamp(int(s))


def get_not_after(comment: str) -> t.Optional[datetime]:
    s = extract_from_comment(comment, "not after")
    if s is None:
        return None
    return datetime.fromtimestamp(int(s))


def main() -> None:
    any_error = False
    key_folder_path = Path(__file__).parent.parent.parent / "key"
    for item in key_folder_path.iterdir():
        try:
            if item.is_file():
                with open(item, encoding="utf8") as f:
                    comment, key = f.readlines()

                if key.endswith("\r\n"):
                    key = key[:-2]
                elif key.endswith("\n"):
                    key = key[:-1]
                key_bytes = base64.b64decode(key, validate=True)
                if len(key_bytes) != 42:
                    raise ValueError("Unknown key format")

                if not comment.startswith("untrusted comment: "):
                    raise ValueError("Unexpected first line: expected untrusted comment")
                if not comment.endswith("\n") and not comment.endswith("\r\n"):
                    raise ValueError("Unexpected line ending after untrusted comment")
                comment = comment.rstrip("\r\n")
                for pos, c in enumerate(comment, 1):
                    if ord(c) < 0x20:
                        raise ValueError(f"Unexpected characters found in untrusted comment at position {pos}")
                comment = comment.split(": ", 1)[1]
                hex_key_id = get_hex_pub_key(comment)
                alias = get_alias(comment)
                not_before = get_not_before(comment)
                not_after = get_not_after(comment)
                if hex_key_id is None:
                    raise ValueError("Expected pub key id in untrusted comment")
                if hex_key_id.upper() != item.stem.upper():
                    raise ValueError("Filename does not match pub key id in untrusted comment")
                if alias is None:
                    raise ValueError("Expected pub key alias in untrusted comment")
                if alias in ("not", "alias", "minisign", "public", "private", "key"):
                    raise ValueError("Invalid pub key alias in untrusted comment")
                if not_before is None:
                    raise ValueError("Expected not before in untrusted comment")
                if not_after is None:
                    raise ValueError("Expected not after in untrusted comment")

                now = datetime.now()
                if not_before >= not_after:
                    raise ValueError("No valid lifespan")
                if not_before - now > timedelta(days=365):
                    raise ValueError("Will only become valid in more than a year")
                if not_before < datetime(year=2021, month=1, day=1):
                    raise ValueError("Key is older than PopTracker")
                if not_after - not_before > timedelta(days=1825):
                    raise ValueError("Key is valid for longer than 5 years")

                if not_after < now:
                    expiration = "expired"
                elif not_before > now:
                    expiration = f"becomes valid in {(not_before - now).days} days"
                else:
                    expiration = f"expires in {(not_after - now).days} days"
                print(f"{hex_key_id}: {alias} {expiration}")

        except ValueError as e:
            print(f"{item.name}: {e}", file=sys.stderr)
            any_error = True

    if any_error:
        exit(1)


if __name__ == "__main__":
    main()
