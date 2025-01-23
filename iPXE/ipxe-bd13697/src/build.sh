#!/bin/bash

LOADERDIR=../../../TARFS/data/loader

if echo $* | grep -P -q '(bios|uefi|ia32|arm64|riscv64)'; then

    if [ "$1" == "bios" ]; then
        rm -f bin-x86_64-pcbios/undionly.kpxe
        rm -rf bin-x86_64-pcbios        
        rm -f ./ipxe.bios.0
        
        make -j16 -e -k bin-x86_64-pcbios/undionly.kpxe BIOS_MODE=BIOS  TOY_ARCH=x86 CPU_TYPE=1

        if [ -e bin-x86_64-pcbios/undionly.kpxe ]; then    
            mv bin-x86_64-pcbios/undionly.kpxe  ./ipxe.bios.0
            /bin/cp -a ./ipxe.bios.0 $LOADERDIR/
        fi
		
		
    elif [ "$1" == "uefi" ]; then
        for bdname in snponly snp ipxe; do
            rm -rf bin-x86_64-efi
            rm -f ./ipxe.x64.${bdname}.efi.0  
            
            make -j16 -e bin-x86_64-efi/${bdname}.efi  BIOS_MODE=UEFI TOY_ARCH=X64 CPU_TYPE=2
            
            if [ -e bin-x86_64-efi/${bdname}.efi ]; then    
                mv bin-x86_64-efi/${bdname}.efi ./ipxe.x64.${bdname}.efi.0 
                /bin/cp -a ./ipxe.x64.${bdname}.efi.0  $LOADERDIR/            
            fi
        done
    
    elif [ "$1" == "ia32" ]; then
        for bdname in snponly snp ipxe; do
        
            rm -rf bin-i386-efi
            rm -f ./ipxe.ia32.${bdname}.efi.0  
            
            make -j16 -e bin-i386-efi/${bdname}.efi  BIOS_MODE=UEFI  TOY_ARCH=IA32 CPU_TYPE=3
            
            if [ -e bin-i386-efi/${bdname}.efi ]; then    
                mv bin-i386-efi/${bdname}.efi ./ipxe.ia32.${bdname}.efi.0
                /bin/cp -a ./ipxe.ia32.${bdname}.efi.0  $LOADERDIR/                  
            fi
        done
         
	elif [ "$1" == "arm64" ]; then
        for bdname in snponly snp ipxe; do
            rm -rf bin-arm64-efi
            rm -f ./ipxe.arm64.${bdname}.efi.0  
            
            make -j16 -e bin-arm64-efi/${bdname}.efi  CROSS_COMPILE=aarch64-linux-gnu-  ARCH=arm64  BIOS_MODE=UEFI TOY_ARCH=ARM64 CPU_TYPE=4
            
            if [ -e bin-arm64-efi/${bdname}.efi ]; then    
                mv bin-arm64-efi/${bdname}.efi ./ipxe.arm64.${bdname}.efi.0 
                /bin/cp -a ./ipxe.arm64.${bdname}.efi.0  $LOADERDIR/
            fi
        done

    elif [ "$1" == "riscv64" ]; then
        for bdname in snponly snp ipxe; do
            rm -rf bin-riscv64-efi
            rm -f ./ipxe.riscv64.${bdname}.efi.0  
            
            make -j16 -e bin-riscv64-efi/${bdname}.efi CROSS_COMPILE=riscv64-linux-gnu- ARCH=riscv BIOS_MODE=UEFI TOY_ARCH=RISCV64 CPU_TYPE=5
            
            if [ -e bin-riscv64-efi/${bdname}.efi ]; then    
                mv bin-riscv64-efi/${bdname}.efi ./ipxe.riscv64.${bdname}.efi.0 
                /bin/cp -a ./ipxe.riscv64.${bdname}.efi.0 $LOADERDIR/
            fi
        done
    fi
else
    echo "Usage: build.sh { bios | uefi | ia32 | arm64 | riscv64 }"
    exit 0
fi
