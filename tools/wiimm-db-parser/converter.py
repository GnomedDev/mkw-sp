from TrackPacks_pb2 import Pack, ProtoSha1, ProtoTrack
from configparser import ConfigParser

original = ConfigParser()
original.read("vanilla.ini")
pack_info = original["Pack Info"]

def parse_tracks(tracks: str) -> list[ProtoSha1]:
    return [ProtoSha1(data=sha.encode()) for sha in tracks.split(",")[:-1]]

pack = Pack(
    name=pack_info["name"],
    authorNames=pack_info["author"],
    description=pack_info["description"],
    raceTracks=parse_tracks(pack_info["race"]),
    coinTracks=parse_tracks(pack_info["coin"]),
    balloonTracks=parse_tracks(pack_info["balloon"]),
    unreleasedTracks=[
        ProtoTrack (
            sha1=ProtoSha1(data=name.encode()),
            name="", # Filled in in game
            slotId=int(section["slot"]),
            type=int(section["type"]),
            musicId=None
        )

        for name, section in list(original.items())[1:]
        if name != "Pack Info"
    ]
)

with open("vanilla.pb.bin", "wb") as file:
    file.write(pack.SerializeToString())
