use std::io::Result;

#[cfg(not(any(feature = "online_gameplay", feature = "pack_downloading", feature = "update")))]
compile_error!("Neither online gameplay, pack downloading, or update features enabled!");

#[cfg(all(feature = "online_gameplay", feature = "pack_downloading"))]
compile_error!("Both online gameplay and pack downloading features enabled!");

fn main() -> Result<()> {
    let files = [
        #[cfg(feature = "update")]
        "../../protobuf/Update.proto",
        #[cfg(feature = "online_gameplay")]
        "../../protobuf/Room.proto",
        #[cfg(feature = "online_gameplay")]
        "../../protobuf/Matchmaking.proto",
        #[cfg(feature = "pack_downloading")]
        "../../protobuf/TrackServer.proto",
    ];

    for file in &files {
        println!("cargo:rerun-if-changed={file}");
    }
    prost_build::compile_protos(&files, &["../../protobuf"])?;
    Ok(())
}
