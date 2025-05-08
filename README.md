Attention that:

iVentoy and Ventoy are two completely different softwares.

Name | Official Website| Open Source|Edition|Use Case
-|-|-|-|-
**Ventoy** | [https://www.ventoy.net](https://www.ventoy.net) | 100% open source|Only open source edition|Install OS through USB/HDisk
**iVentoy** | [https://www.iventoy.com](https://www.iventoy.com) | part open source <br>part closed source|Free-Edition <br>Pro-Edition|Install OS through network(PXE)

This repo only contains the open source part of iVentoy.

So you should decide to use it or not.




When install Windows, iVentoy will load httpdisk.sys in the WinPE environment.

httpdisk is an open source project: https://www.accum.se/~bosse/httpdisk/httpdisk-10.2.zip

This driver is signed with WDKTestCert.

This driver is used to mount the ISO file in the server side as a local drive (e.g. Y:) throug http.

This driver will only be installed in the temporary WinPE environment and will not be installed to the final Windows system in the hardisk.

This driver will only exist in RAM temporary during installation and will disappear after finish the installation and reboot.

When install Windows 11, iVentoy will create the following registry keys to make old hardware can install Windows 11.
LabConfig - The registry key in which these flags need to be set
BypassTPMCheck - Bypasses Windows 11's check for TPM 2.0
BypassSecureBootCheck - Bypasses Windows 11's check for the Secure Boot status in the UEFI
BypassNRO - Bypasses the Microsoft Account requirement in the Out-of-Box Experience, letting you create a local user account.
