#!/bin/sh
warning(){
    echo "-n: Bu parametre zorunludur. Uygulama ismini içermelidir. Örneğin ecl, lmm vb. gibi."
    echo "-d: Bu parametre zorunludur. Build edilecek cihaz bilgisini içermelidir.
    Kullanılması gereken parametreler;
    * Nvidia Tx1 cihazlar için 'arm64-tx1'
    * Nvidia Jetson Tk1 için 'arm32-jtk1'
    * x86_64(desktop) Ubuntu 16.04 için 'amd64-16.04'
    * x86_64(desktop) Ubuntu 18.04 için 'amd64-18.04' olmak zorundadır."
    echo "Kullanım notları:
    * Build klasör yollarını ASLA değiştirmeyin. Build işlemi sonucu oluşan dosyaları
    \"artifact\" olarak aldığımız için \"artifact\" dosya yolu bu dizinlere bağlıdır.
    * Dosya içerisinde bağımlılıkları indirme ve build işlemleri dışında herhangi birşeyi 
    ASLA değiştirmeyin. Aksi durumda CI/CD işlemleri hatalı sonuçlanacaktır.
    (Örneğin; Projenizin Jetson TK1 desteği olmasını istemiyorsanız bile \"jtk1build\" fonksiyonunu silmeyiniz.)"    
}

if [ $# -eq 0 ]
  then
    warning
    exit
fi

NAME=""
DEVICE=""

while getopts n:d: option
do
case "${option}"
in
    n) NAME=${OPTARG}
        ;;
    d) DEVICE=${OPTARG}
        ;;

esac
done

choose_device() {
    if [ $DEVICE = "arm64-tx1" ]
        then
            tx1build
    elif [ $DEVICE = "arm32-jtk1" ]
        then
            jtk1build
    elif [ $DEVICE = "amd64-16.04" ]
        then
            amd64_16
    elif [ $DEVICE = "amd64-18.04" ]
        then
            amd64_18
    else warning
    
    fi
}

tx1build() {
apt update
apt install -y qt5-default make g++ wget

wget https://gitlab.com/ozogulf/ci-files/raw/master/tx1/grpc_TX1.tar.gz
tar xf grpc_TX1.tar.gz -C /usr/local

ldconfig

mkdir -p build/ptzp
cd build/ptzp
qmake ../../
make -j4
}

amd64_16() {

apt update
apt install -y qt5-default make g++ wget

wget https://gitlab.com/ozogulf/ci-files/raw/master/x86_64/grpc_x86_64_16_04.tar.gz
tar xf grpc_TX1.tar.gz -C /usr/local

ldconfig

mkdir -p build/ptzp
cd build/ptzp
qmake ../../
make -j4

}

amd64_18() {

apt update
apt install -y qt5-default make g++ wget

wget https://gitlab.com/ozogulf/ci-files/raw/master/x86_64/grpc_x86_64_16_04.tar.gz
tar xf grpc_TX1.tar.gz -C /usr/local

ldconfig

mkdir -p build/ptzp
cd build/ptzp
qmake ../../
make -j4
}

jtk1build() {
mkdir -p build/ptzp
cd build/ptzp
qmake ../../
make -j4

}
