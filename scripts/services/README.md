# Sample services for Raspberry Pi

Those two service files can be installed on a Raspberry Pi and will automatically start/restart both goesrecv and goesproc at boot time.

By default they will use configuration files in /home/pi and save incoming data in /home/pi/incoming, but you can of course adjust those before installing the services.

## How to install

After editing them to adjust paths if necessary, copy both service files to `/etc/systemd/system`. Make sure the configuration files exist, and the `incoming` directory is created at the location referenced in the service description file.

You should then test if the service starts properly by using `sudo systemctl start goesrecv.service`  and `sudo systemctl start goesproc.service`.

If all goes well, then you can simply enable the services so that they run at boot time:

`
sudo systemctl enable goesrecv.service
sudo systemctl enable goesproc.service
`
