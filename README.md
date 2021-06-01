# BAudio: Bijan Audio Driver
BAudio is a workaround to connect XLR based capture card to Dragon Naturrally Speaking. It is now in the devolopment phase and targeted on Win 7 x64.
BAudio is built upon WDK 10.0.19041, Visual Studio 2019 and is a KMDF driver.

## Installation
### Enable Test Signing
If you know how I can sign this driver please contact with me, For the time being to use woithout signing enable test signing in the cmd with
```
bcdedit /set testsigning on
```

### Setup Driver
1. Open Device Manager
2. From Menubar: `Action > Add legacy hardware`
3. Select `Install the hardware that I manually select ...`
4. Click `Have Disk` and Browser for the inf file
