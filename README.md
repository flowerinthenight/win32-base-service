# Skeleton Win32 service

### ETW trace

This project uses the ETW setup [here](https://github.com/flowerinthenight/win32-etw-manifest).

### To install the service

An installer project `svcsetup` is included.

```
svcsetup.exe install
```

Open `Services` management console. A service with the name `win32basesvc` should now be installed. If it's not started, start it.

### Test service installation

#### Using sc.exe control

Open command prompt and execute the command

```
sc.exe control win32basesvc 128
```

The control code will be logged using ETW.

Valid user custom control codes are from 128 - 255. More information [here](http://msdn.microsoft.com/en-us/library/windows/desktop/ms683241(v=vs.85).aspx).

#### Using Win+Lock/Unlock

While the service is running, perform a system lock (Win + L) and an unlock. Lock and unlock control codes will be logged using ETW as well.

### To uninstall the service

Stop the service from the `Services` management control. Then uninstall it using `svcsetup`.

```
svcsetup.exe uninstall
```

# License

[The MIT License](./LICENSE.md)
