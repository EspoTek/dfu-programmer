int dfuprog_virtual_cmd(char* commandLine, libusb_device *device, libusb_device_handle *handle, libusb_context *parentContext, int32_t interface);
int dfuprog_virtual_main(int argc, char **argv, libusb_device *device, libusb_device_handle *handle, libusb_context *parentContext, int32_t interface);
int main( int argc, char **argv );
