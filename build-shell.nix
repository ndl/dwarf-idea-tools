with (import <nixpkgs> {});
mkShell {
  buildInputs = [
    bzip2
    cmake
    gcc
    ninja
    zlib
    xz
  ];
}
