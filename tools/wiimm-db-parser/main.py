import csv
from typing import Generator

from TrackPacks_pb2 import ProtoSha1, TrackDB, ProtoTrack as Track
AliasValue = TrackDB.AliasValue

@staticmethod
def track_from_csv(line: list[str]) -> Track:
    sha1, wiimmId, _, _, ctype, slot, music_slot, _, _, prefix, name, _, _, _, _, _ = line

    if len(prefix) != 0:
        name = f"{prefix} {name}"

    return Track(
        sha1=ProtoSha1(data=sha1.encode()),
        name=name,
        slotId=int(slot),
        type=int(ctype),
        musicId=int(music_slot),
        wiimmId=int(wiimmId)
    )

def read_wiimm_csv(path: str) -> Generator[Track, None, None]:
    with open(path) as db_csv:
        db_csv.readline()

        reader = csv.reader(db_csv, delimiter="|")
        yield from (track_from_csv(line) for line in reader)

def read_aliases_csv(tracks: list[Track]) -> list[AliasValue]:
    aliases = []
    tracks_sha1 = {t.wiimmId: (t.sha1, t.slotId) for t in tracks}

    with open("sha1-reference.txt") as alias_csv:
        for _ in range(9):
            alias_csv.readline()

        reader = csv.reader(alias_csv, delimiter="|")
        for line in reader:
            if line == [] or line[0] == "":
                continue

            alias_sha1, wiimmId, _, _, cflags, _ = line
            wiimmId = int(wiimmId)
            if (
                all(map(lambda f: f not in cflags, "ZPd"))
                and tracks_sha1[wiimmId][0].data != alias_sha1.encode()
                and tracks_sha1[wiimmId][1] != 0
            ):
                alias_value = AliasValue(
                    aliased=ProtoSha1(data=alias_sha1.encode()),
                    real=tracks_sha1[wiimmId][0]
                )

                aliases.append(alias_value)

    return aliases

def main():
    tracks = list(read_wiimm_csv("extended-tracks.csv"))

    try:
        tracks.extend(read_wiimm_csv("public-ref.list"))
    except FileNotFoundError:
        print("Warning: Could not find public-ref.list, output will only include out-of-db tracks!")
        print("Warning: You can download it from http://archive.tock.eu/wbz/public-ref.list\n")

    trackDB = TrackDB(
        tracks=[t for t in tracks if t != 0],
        aliases=read_aliases_csv(tracks)
    )

    with open("tracks.pb.bin", "wb") as tracks_bin:
        print(f"Finished writing {len(tracks)} tracks and {len(trackDB.aliases)} aliases to tracks.pb.bin!")
        tracks_bin.write(trackDB.SerializeToString())

if __name__ == "__main__":
    main()
