{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    stdenv
    cmake
    (writeShellScriptBin "recompile" ''
      #!/bin/sh
      set -e  # Exit immediately if a command exits with a non-zero status.

      echo "Compiling server..."
      gcc -o server server.c -lpthread

      echo "Compiling client..."
      gcc -o client client.c

      echo "Both server and client are compiled. You can run './server' and './client <server IP> <server Port>'."
      echo "Example for client:"
      echo "  ./client 127.0.0.1 5000"
    '')
  ];

  shellHook = ''
    echo "Environment ready."
    echo "To compile the server and client, use the custom 'recompile' pkg that comes with this shell."

    echo "You can then run the server with './server' [PORT] and the client with './client [server IP] [PORT]'."
    echo "Example for client:"
    echo "  ./client 127.0.0.1 5000"
  '';
}
