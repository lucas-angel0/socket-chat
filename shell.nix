{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  buildInputs = [ pkgs.gcc pkgs.stdenv pkgs.cmake ];

  shellHook = ''
    echo "Environment ready."
    echo "To compile the server and client, run:"
    echo "  gcc -o server server.c -lpthread"
    echo "  gcc -o client client.c"
    echo "You can then run the server with './server' and the client with './client [server IP] [server Port]'."
    echo "Example for client:"
    echo "  ./client 127.0.0.1 5000"
  '';
}
