import contextlib
import csv
import os
from typing import Generator

from TrackPacks_pb2 import ProtoSha1, AliasDB, ProtoTrack as Track
AliasValue = AliasDB.AliasValue

class ShaTrack:
    sha1: ProtoSha1
    inner: Track

    def __init__(self, sha1, inner):
        self.sha1 = sha1
        self.inner = inner

def track_from_csv(line: list[str]) -> ShaTrack:
    sha1, wiimmId, _, _, ctype, slot, music_slot, _, _, prefix, name, _, _, _, _, _ = line

    if len(prefix) != 0:
        name = f"{prefix} {name}"

    return ShaTrack(
        sha1=ProtoSha1(data=bytes.fromhex(sha1)),
        inner=Track(
            name=name[:40],
            slotId=int(slot),
            type=int(ctype),
            musicId=int(music_slot),
            wiimmId=int(wiimmId)
        )
    )

def read_wiimm_csv(path: str) -> Generator[ShaTrack, None, None]:
    with open(path) as db_csv:
        db_csv.readline()

        reader = csv.reader(db_csv, delimiter="|")
        yield from (track_from_csv(line) for line in reader)

def read_aliases_csv(tracks: list[ShaTrack]) -> list[AliasValue]:
    aliases = []
    tracks_sha1 = {t.inner.wiimmId: (t.sha1, t.inner.slotId) for t in tracks}

    with open("in/sha1-reference.txt") as alias_csv:
        for _ in range(10):
            alias_csv.readline()

        reader = csv.reader(alias_csv, delimiter="|")
        for line in reader:
            if line == [] or line[0] == "":
                continue

            alias_sha1, wiimmId, _, _, cflags, _ = line
            wiimmId = int(wiimmId)
            if (
                all(map(lambda f: f not in cflags, "ZPd"))
                and tracks_sha1[wiimmId][0].data != bytes.fromhex(alias_sha1)
                and tracks_sha1[wiimmId][1] != 0
            ):
                alias_value = AliasValue(
                    aliased=ProtoSha1(data=bytes.fromhex(alias_sha1)),
                    real=tracks_sha1[wiimmId][0]
                )

                aliases.append(alias_value)

    return aliases

def main():
    tracks = list(read_wiimm_csv("extended-tracks.csv"))

    try:
        tracks.extend([t for t in read_wiimm_csv("in/public-ref.list") if t not in tracks])
    except FileNotFoundError:
        print("Warning: Could not find public-ref.list, output will only include out-of-db tracks!")
        print("Warning: You can download it from http://archive.tock.eu/wbz/public-ref.list\n")

    with contextlib.suppress(FileExistsError):
        os.mkdir("out")
    with contextlib.suppress(FileExistsError):
        os.mkdir("out/tracks")

    track_count = 0
    for track in tracks:
        if track.inner.slotId == 0:
            continue

        track_count += 1
        track_sha1 = track.sha1.data.hex()
        with open(f"out/tracks/{track_sha1}.pb.bin", "wb") as track_file:
            track_file.write(track.inner.SerializeToString())

    print(f"Finished writing {track_count} tracks to tracks directory")

    alias_db = AliasDB(aliases=read_aliases_csv(tracks))
    with open("out/alias.pb.bin", "wb") as alias_file:
        alias_file.write(alias_db.SerializeToString())

if __name__ == "__main__":
    main()
