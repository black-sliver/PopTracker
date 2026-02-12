import os
import subprocess
from pathlib import Path
from shutil import which
from zipfile import ZipFile, ZipInfo, ZIP_STORED, ZIP_DEFLATED

data_folder = Path(__file__).parent.parent / "data"

zip_data = {
    "empty": b"",
    "very_short": b"1111",
    "short": b"1234" * 1024 + b"5" + b"6789" * 1023 + b"555",
}

if __name__ == "__main__":
    # generate a regular (non-ZIP64) zip with regular (non-ZIP64) central directory with some STORE and DEFLAT files
    with ZipFile(data_folder / "sample.zip", "w", allowZip64=False) as zf:
        for compression in (0, 1, 9):
            compress_type = ZIP_DEFLATED if compression else ZIP_STORED
            # create folder, set date and time to all zeroes
            zi = ZipInfo(f"{compression}/", (1980, 0, 0, 0, 0, 0))
            zi.CRC = 0
            zf.mkdir(zi)
            for name, data in zip_data.items():
                if compression == 0 and len(data) > 64:
                    continue  # no point in testing this
                # create file, set date and time to all zeroes
                zi = ZipInfo(f"{compression}/{name}.txt")._for_archive(zf)
                zi.date_time = (1980, 0, 0, 0, 0, 0)
                zf.writestr(zi, data, compress_type, compression)
    # generate a regular (non-ZIP64) zip file with ZIP64 file header
    with ZipFile(data_folder / "hdr64.zip", "w", allowZip64=True) as zf:
        zi = ZipInfo("empty.txt")._for_archive(zf)
        zi.date_time = (1980, 0, 0, 0, 0, 0)  # set date and time to all zeroes
        with zf.open(zi, "w", force_zip64=True) as ze:
            pass  # empty file
    # generate a regular (non-ZIP64) zip file with a DEFLATE64 file and ZIP64 cd; DEFLATE64 is unsupported by PopTracker
    exe = which("7z") or which("7za")
    filename = data_folder / "deflate64.zip"
    filename.unlink(missing_ok=True)
    subprocess.run((exe, "a", "-mm=deflate64", filename, "-si-"), input=b"\x00" * 64)
    # update timestamp
    with open(filename, "r+b") as f:
        if f.read(4) != b"PK\x03\x04":
            raise ValueError("Unexpected start")
        f.seek(10, os.SEEK_SET)
        f.write(b"\x00\x00\x00\x00")
        f.seek(-22, os.SEEK_END)
        eocd = f.read(22)
        if eocd[:4] != b"PK\x05\x06":
            raise ValueError(f"Unexpected EOCD: {eocd[:4]}...")
        start_of_cd = eocd[16] + (eocd[17] << 8) + (eocd[18] << 16) + (eocd[19] << 24)
        if start_of_cd == 0xffffff:
            raise ValueError("Unexpected ZIP64")
        f.seek(start_of_cd, os.SEEK_SET)
        if f.read(4) != b"PK\x01\x02":
            raise ValueError("Bad cd")
        f.seek(start_of_cd + 12, os.SEEK_SET)
        f.write(b"\x00\x00\x00\x00")
        # delete extra field in cd
        f.seek(start_of_cd + 28, os.SEEK_SET)
        b = f.read(2)
        filename_len = b[0] + (b[1] << 8)
        f.seek(start_of_cd + 30, os.SEEK_SET)
        b = f.read(2)
        extra_len = b[0] + (b[1] << 8)
        if extra_len:
            f.seek(start_of_cd + 30, os.SEEK_SET)
            f.write(b"\x00\x00")
            f.seek(start_of_cd + 46 + filename_len + extra_len, os.SEEK_SET)
            rest = f.read()
            f.seek(start_of_cd + 46 + filename_len, os.SEEK_SET)
            f.write(rest)
            f.truncate()
            # shrink cd
            f.seek(-22 + 12, os.SEEK_END)
            b = f.read(4)
            cd_len = b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24)
            if cd_len == 0xffffffff:
                raise ValueError("Unexpected ZIP64")
            cd_len -= extra_len
            f.seek(-22 + 12, os.SEEK_END)
            f.write(bytes((cd_len & 0xff, (cd_len >> 8) & 0xff, (cd_len >> 16) & 0xff, (cd_len >> 24) & 0xff)))

    # TODO: generate a ZIP64 zip file with ZIP64 central directory
    # TODO: generate a ZIP64 zip file with ZIP64 EOCD
