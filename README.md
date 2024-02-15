Instructions to run the program:
Build the container:
    docker build -t <imageName> .
    docker run -v <pathToGitProject>:<pathToMount> -it /bin/bash <imageName>
Inside the container, to build the program:
    cd <pathToMount>
    mkdir build
    cd build
    cmake ..
    make