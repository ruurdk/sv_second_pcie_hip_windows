# Enabling the second, hidden PCIe HIP on 5SGSMD5K1F40C1 (5SGSKF40I3LNAC) in Quartus Windows

For Linux refer to [this repo](https://github.com/ruurdk/sv_second_pcie_hip/tree/main). 

## Getting started

Here you'll just find the technical how-to. For documentation of how this works [check this blog](https://devops.lol/azure-fpga).

0. Prerequisites - Install Visual Studio Community Edition (tested with 2022)
1. Building: open the solution file, and build it.
2. Go to the quartus binary folder (e.g. C:\intelFPGA\22.1std\quartus\bin64), and change the ddb_dev.dll name to ddb_dev_orig.dll

```
mv ddb_dev.dll ddb_dev_orig.dll
```

3. Copy the build output file `ddb_dev.dll` to the quartus binary folder (see above).
4. Launch Quartus, 
5. Instantiate a second PCIe HIP in the design; it'll be placed in the right spot based on the reference clock I/O constraint

## How does this work?

The original code for Linux depends on LD_PRELOAD to override a single library function by preloading a library with just that function.
There is no such thing in Windows (yes there were appinit dlls but we all know that story).
The solution presented here depends on creating a wrapper library: we export and forward every function found in the target library to the original library, and only implement the changed function.

To do this:

- the exporter project creates an executable that can extract dll exports, and format them in the style of linker commands - you don't have to do that yourself, project is just here for reference
- the ddb_dev project uses the output from the above (ddb_dev.cpp) to forward all functions but one to the original library, and implement the single function we want to alter