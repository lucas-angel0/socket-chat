
{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  buildInputs = [ pkgs.gcc pkgs.stdenv pkgs.cmake ];

  shellHook = ''
    echo "Environment ready."
    echo "To compile the server and client, use the alias 'recompile'."

    # Create the recompile script
    cat > recompile <<'EOF'
    #!/usr/bin/env sh
    echo "Compiling server..."
    gcc -o server server.c -lpthread
    echo "Compiling client..."
    gcc -o client client.c
    EOF

    # Make the script executable
    chmod +x recompile

    # Add the script to the PATH
    export PATH=$PWD:$PATH

    echo "You can then run the server with './server' and the client with './client [server IP] [server Port]'."
    echo "Example for client:"
    echo "  ./client 127.0.0.1 5000"
  '';
}
