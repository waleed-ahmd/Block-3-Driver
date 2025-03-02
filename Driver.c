#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h> //for device registration
#include <linux/uaccess.h> //provides functions to copy data from user space
#include <linux/proc_fs.h> //for proc file
#include <linux/usb.h> //for usb interaction
//april added a comment

#define DEVICE_NAME "Wacom-Tablet" //name of device
#define BUFFER_SIZE 1024 //size of internal buffer

//vendor and product ID of wacom tablet gotten from lsusb
#define DEVICE_VENDOR_ID 0x056a
#define DEVICE_PRODUCT_ID 0x033b
//bloody usbhid keeps stealing my wacom away from me
#define DEVICE_CLASS_ID 3
#define DEVICE_SUBCLASS_ID 1
#define DEVICE_PROTOCOL_ID 2
//proc file system name
#define proc_name "wacom-device-tablet"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yasmin");
MODULE_DESCRIPTION("Wacom tablet device driver.");
MODULE_VERSION("1.0");

static int major_number; //stores dynamic allocated major number.
static char buffer [BUFFER_SIZE]; //internal buffer size
static size_t buffer_data_size = 0; //keeps track of how much data is stored in the buffer


//declaring usb_driver
static struct usb_driver usb_driver;

//last argument always has to be empty
static struct usb_device_id usb_table[] = 
{
    {USB_DEVICE(DEVICE_VENDOR_ID, DEVICE_PRODUCT_ID)}, 
		{USB_DEVICE_INFO(DEVICE_CLASS_ID, DEVICE_SUBCLASS_ID, DEVICE_PROTOCOL_ID)},
    {}, 
};
MODULE_DEVICE_TABLE(usb, usb_table); 

//this function is executed when the usb is put in
static int usb_probe(struct usb_interface *intf, const struct usb_device_id *id) { 
		//usbhid keeps auto claiming the device so I need to forcefully claim it
		int return_value = usb_driver_claim_interface(&usb_driver, intf, NULL);
   		if (return_value) {
   				printk(KERN_ERR "Failed to claim interface\n");
       		return -return_value;
 			}

		printk(KERN_INFO "Probe function executed: vendor=0x%x, product=0x%x\n", id->idVendor, id->idProduct);
    return 0;
}

//this function is executed when the usb is ejected out 
static void usb_disconnect(struct usb_interface *intf) { 
    printk("The Exit function has executed"); 
		usb_driver_release_interface(&usb_driver, intf);
}

static struct usb_driver usb_driver =
{
    .name = "WacomDeviceDriver",
    .id_table = usb_table,
    .probe = usb_probe,
    .disconnect = usb_disconnect
};

//Shows that device is opened in kernel
static int device_open(struct inode *inode, struct file *file) {
	printk(KERN_INFO "Device opened\n");
	return 0;
}

//Shows that device is released in kernel
static int device_release(struct inode *inode, struct file *file) {
	printk(KERN_INFO "Device released\n");
	return 0;
}

//function to handle read operations
static ssize_t device_read(struct file *file, char __user *user_buffer, size_t len, loff_t *offset){
	//determine minimum of requested length and available data
	size_t bytes_to_read = min(len, buffer_data_size);
	//copy data in to user space
	if (copy_to_user(user_buffer, buffer, bytes_to_read)) {
		//if buffer is too big to copy to user
		return -EFAULT;
	}
	//refresh data in buffer
	buffer_data_size = 0;
	//log device upon read
	printk(KERN_INFO "Device read %zu bytes\n", (size_t)bytes_to_read);
	return bytes_to_read;
}

//function to handle write operations
static ssize_t device_write(struct file *file, const char __user *user_buffer, size_t len, loff_t *offset){
	size_t bytes_to_write = min(len, (size_t)(BUFFER_SIZE -1));

	if(copy_from_user(buffer, user_buffer, bytes_to_write)){
		return -EFAULT;
	}
	buffer[bytes_to_write] = '\0'; //terminates program if nothing to write
	buffer_data_size = bytes_to_write;

	printk(KERN_INFO "Device wrote %zu bytes.\n", (size_t)bytes_to_write);

	return bytes_to_write;
}

static struct file_operations fops={
	.open = device_open,
	.release = device_release,
	.read = device_read,
	.write = device_write,
};

static int __init loopback_init(void){

  int usb_result;
	/*
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if(major_number < 0){
		printk(KERN_ALERT "Failed to register major number\n");
		return major_number;
	}
	printk(KERN_INFO "%s device registered with major number %d\n", DEVICE_NAME, major_number);
	*/
		usb_result = usb_register(&usb_driver);
		if(usb_result){
	   	 printk("error loading register");
	   	 return -usb_result;
		}

		return 0;
}

static void __exit loopback_exit(void){

		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_INFO "%s device unregistered\n", DEVICE_NAME);
		usb_deregister(&usb_driver);
		}


// Register module entry and exit points
module_init(loopback_init);
module_exit(loopback_exit);
