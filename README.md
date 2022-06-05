# simfan
### simfan is a simple hwmon fan controller 
#### A very no thrills fan controller wrtten in C to use very little resources
##### Refer to [simfan.conf](/simfan.conf.example) for all available features
## Dependencies
- libconfig
  - Because I'm too lazy to make my own config parser
  
 ## Building
#### To build:
```
git clone --single-branch https://github.com/GhostNaN/simfan
cd simfan
meson build --prefix=/usr/local
ninja -C build
```
#### To install:
```
ninja -C build install
cp simfan.conf.example /etc
```

## Usage 
Root privilege to run simfan is most likely necessary (AT YOUR OWN RISK)
#### To Run:
```
simfan
```
#### For help:
```
simfan -h
```
Everything else you'll need is in [simfan.conf](/simfan.conf.example)

 ## License
This project is licensed under the GPLv3 License - see the [LICENSE](/LICENSE) file for details
