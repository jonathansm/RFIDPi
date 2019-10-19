# RFIDPi
The setup I used is a Raspberry Pi Zero W running the latest Raspbian Stretch 

## Setup
### Step 1
First, we need to install some packages

```
sudo apt-get install dnsmasq 
sudo apt-get install hostapd
sudo apt-get install nginx
sudo apt-get install python3
```
We are first installing a utility called `dnsmasq` this will allow the Raspberry Pi to hand out IP addresses to any clients that connect to its wireless network. That is where `hostapd` comes in. That allows the Raspberry Pi to act as a WiFi access point. This will allow our device to connect to it to access the web page. `nginx` is a web server, you can use Apache if you want if you do your instructions will be a little different. Last we install Python; this will allow us to run the API and database.

### Step 2
To make things easy, you can copy over the configuration files I created. You can run these commands if you are in the `config` directory.

```
cp dnsmasq.conf /etc/dnsmasq.conf
cp hostapd.conf /etc/hostapd/hostapd.conf
```

### Step 3
Next, we need to move the website into the correct directory for `nginx`. You can run these commands if you are in the main project directory.

```
cp -a server/web/. /var/www/html/
```

### Step 4
Now we need to start all the services.

```
sudo systemctl start hostapd
sudo systemctl start dnsmasq
sudo systemctl start nginx
```
After these have started make sure you see and can connect to a wireless network called `RDIDPi` the password is `RFIDPi20!!!` you can change if you want. It is located in the `/etc/hostapd/hostapd.conf` file.

### Step 5
Now you need to copy the rest of the scripts/application. You can put them where ever you want. I have them in the `pi` user directory to make things easy. You can run these commands if you are in the main project directory.

```
mkdir ~/rfidpi
cp -a server/api/. ~/rfidpi
cp -a driver/reader ~/rfidpi
```

### Step 6
Last we need our API and driver code to start up when the Raspberry Pi starts up. Run `crontab -e` and add the following to the file.

```
@reboot /home/pi/rfidpi/reader &
@reboot python3 /home/pi/rfidpi/rfidpi_rest_api.py &

```