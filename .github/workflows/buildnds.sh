source /etc/profile.d/devkit-env.sh

apt-get update && apt-get install p7zip-full zip libc6-dev g++ -y

cd /build_dir

#Build nds
make nds
