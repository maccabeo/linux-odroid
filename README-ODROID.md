# Vanilla Linux 3.17.y for the ODROID-X2

This is a [linux-3.17.y](https://git.kernel.org/cgit/linux/kernel/git/stable/linux-stable.git/log/?h=linux-3.17.y) tree with some modifications to make it work on a Hardkernel ODROID-X2 developer board.


TODOs:

   - Mali code is merged, but still needs large amount of patching (DRM side mostly)
   - HDMI needs proper fix for powerdomain issue
   - IOMMU is not functional at the moment (this also breaks the userptr feature of the G2D subsystem)
   - remove more of the always-on properties of the various regulators
   - modify refclk code in usb3503 (make it more generic)
