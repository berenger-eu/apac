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


When executing passes (individually) :
    passName targetFile [moreTargerFiles]
    - -main mainName, to set the function where the main taskgroup will be created
    - -functions    , to only parse chosen functions
    - -ignore       , to ignore specified functions
Example : ./myPass myFile -main differentMainName -functions func1,func2,func3 -ignore func4