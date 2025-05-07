When install Windows, iVentoy will load httpdisk.sys in the WinPE environment.

httpdisk is an open source project: https://www.accum.se/~bosse/httpdisk/httpdisk-10.2.zip

This driver is signed with WDKTestCert.

This driver is used to mount the ISO file in the server side as a local drive (e.g. Y:) throug http.

This driver will only be installed in the temporary WinPE environment and will not be installed to the final Windows system in the hardisk.

This driver will disappear after finish the installation and reboot.

