apt update
apt install -y qt5-default make g++ wget

wget https://gitlab.com/ozogulf/ci-files/raw/master/tx1/grpc_TX1.tar.gz
tar xf grpc_TX1.tar.gz -C /usr/local

ldconfig

mkdir -p build/ptzp
cd build/ptzp
qmake ../../
make -j4
