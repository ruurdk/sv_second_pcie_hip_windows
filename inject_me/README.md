# Short walkthrough on how to enable second, hidden PCIe HIP on 5SGSMD5K1F40C1 (5SGSKF40I3LNAC)

1. Open this project in Visual Studio

2. Compile it to create a `.dll` file:

3. Launch Quartus with `LD_PRELOAD` to enable second HIP:

```
LD_PRELOAD=`pwd`/preload_me.dll /path_to_quartus/intelFPGA/version/quartus/bin/quartus
```

4. Instantiate a second PCIe HIP in the design; it'll be placed in the right spot based on the reference clock I/O contstraint
