# Attention:

iVentoy and Ventoy are **two completely different** products.

Name | Official Website| Open Source|Edition|Use Case
-|-|-|-|-
**Ventoy** | [https://www.ventoy.net](https://www.ventoy.net) | 100% open source|Only open source edition|Install OS through USB/HDisk
**iVentoy** | [https://www.iventoy.com](https://www.iventoy.com) | part open source <br>part closed source|Free-Edition <br>Pro-Edition|Install OS through network(PXE)

This repository only contains the open source part of iVentoy, so it is up to you whether you should use it or not.






When installing Windows, iVentoy will load httpdisk.sys in the WinPE environment.

[httpdisk](https://www.accum.se/~bosse/httpdisk/httpdisk-10.2.zip) is an open source project

This driver is signed with WDKTestCert and is used to mount the ISO file in the server side as a local drive (e.g. Y:) through HTTP.

It will only be installed in the temporary WinPE environment and not in the final Windows system on the hard drive.

This driver will only exist in RAM temporarily during installation and will disappear after the installation is finished or the computer is rebooted.

When installing Windows 11, iVentoy will create the following registry keys to make older hardware install Windows 11.

LabConfig - The registry key in which these flags need to be set

BypassTPMCheck - Bypasses Windows 11's check for TPM 2.0

BypassSecureBootCheck - Bypasses Windows 11's check for Secure Boot status in the UEFI

BypassNRO - Bypasses the Microsoft Account requirement in the Out-of-Box Experience (OOBE), letting you create a local user account.

