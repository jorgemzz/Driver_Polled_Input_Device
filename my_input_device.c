#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h> /* struct file_operations, struct file, ... */
#include <linux/types.h> /* ssize_t, dev_t, ... */
#include <linux/slab.h> /* kmalloc() */

#include <linux/i2c.h>  /* struct i2c_device_id, ... */

#include <linux/input.h>
#include <linux/input-polldev.h>

/* 
 * The structure to represent 'in_dev' devices.
 */

struct in_dev {
    struct i2c_client *client;
    struct input_polled_dev *polled_input;
};

#define INPUT_DEVICE_NAME     "in_dev"


void indev_read_registers(struct i2c_client *client, char *buf, int count){
    u8 reg_addr;
    struct i2c_msg msg[2];

    reg_addr = 0x20;    //Read keys status command

    msg[0].addr = client->addr;
    msg[0].flags = 0;                    /* Write */
    msg[0].len = 1;                      /* Address to read */
    msg[0].buf = &reg_addr; 

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;             /* Read */
    msg[1].len = count; //count; 
    msg[1].buf = buf;

    if (i2c_transfer(client->adapter, msg, 2) < 0)
        pr_err("my input device: i2c_transfer failed\n");
}

bool btn0Pressed(unsigned char *buff){
    bool state=0;
    state = *buff & (1);
    return state;
}

bool btn1Pressed(unsigned char *buff){
    bool state=0;
    state = *buff & (1<<1);
    return state;
}

bool btn2Pressed(unsigned char *buff){
    bool state=0;
    state = *buff & (1<<2);
    return state;
}

void indev_poll(struct input_polled_dev *polled_input){
    struct in_dev *in_device = polled_input->private;
    struct i2c_client *client = in_device->client;

    unsigned char buff[1];
    bool btn0Press = 0;
    bool btn1Press = 0;
    bool btn2Press = 0;

    indev_read_registers(client, buff, 1);
    btn0Press = btn0Pressed(buff);
    btn1Press = btn1Pressed(buff);
    btn2Press = btn2Pressed(buff);

    input_event(polled_input->input, EV_KEY, BTN_0, btn0Press);
    input_event(polled_input->input, EV_KEY, BTN_1, btn1Press);
    input_event(polled_input->input, EV_KEY, BTN_2, btn2Press);
    input_sync(polled_input->input);

}

static int indev_probe(struct i2c_client *client, const struct i2c_device_id *id){
    int err = 0;
    struct in_dev *in_device = NULL;

    unsigned char data[1];
    struct input_polled_dev *polled_input = NULL;
    struct input_dev *input = NULL;
    int result;

    if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -EIO;

    /* Test communication with my Input Device */
    indev_read_registers(client, data,1);

    if(btn0Pressed(data))
        pr_info("DBG: BTN_0 pressed!\n");
    if(btn1Pressed(data))
        pr_info("DBG: BTN_1 pressed!\n");
    if(btn2Pressed(data))
        pr_info("DBG: BTN_2 pressed!\n");

    /* Allocate memory when the device is opened the first time */
     in_device = (struct in_dev *)kzalloc(sizeof(struct in_dev), GFP_KERNEL);
    if (in_device == NULL) {
        err = -ENOMEM;
        goto fail;
    }

    /* Polled Input Device initialization */
    polled_input = input_allocate_polled_device();
    if (!polled_input) {
       pr_err("Failed to allocate memory for polling device\n");
        return -ENOMEM;
    }

    polled_input->poll_interval = 50;
    polled_input->poll = indev_poll;
    polled_input->private = in_device;

    in_device->polled_input = polled_input;

    input = polled_input->input;
    input->name = "My input device";

    /* The gpio belong to an expander sitting on I2C */
    input->id.bustype = BUS_I2C;
    input->dev.parent = &client->dev;

    /* Declare the events generated by this driver */
    set_bit(EV_KEY, input->evbit);
    set_bit(BTN_0, input->keybit);
    set_bit(BTN_1, input->keybit);
    set_bit(BTN_2, input->keybit);

    /* Polled Input Device registration */
    result = input_register_polled_device(polled_input);
    if(result<0) {
        input_free_polled_device(polled_input);
        pr_err("Failed to register polled device\n");
        return -ENOMEM;
    }

    in_device->client = client;

    i2c_set_clientdata(client, in_device);

    pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);
    return 0;

fail:
    if (in_device != NULL)
        kfree(in_device);
    return err;
}

static int indev_remove(struct i2c_client *client){
    struct in_dev *my_dev = NULL;
    dev_t curr_dev = 0;
    struct input_polled_dev *polled_input = NULL;
    
    my_dev = i2c_get_clientdata(client);
    if (my_dev == NULL){
        pr_err("Container_of did not found any valid data\n");
        return -ENODEV; /* No such device */
    }

    /* Unregister the polled device and free its allocated memory */
    polled_input = my_dev->polled_input;
    input_unregister_polled_device(polled_input);
    input_free_polled_device(polled_input);
    
    kfree(my_dev);

    /* Freeing the allocated device */
    unregister_chrdev_region(curr_dev, 1);

    pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);

    return 0;
}

static const struct i2c_device_id indev_id[] = {
    {"my_input_device",0},
    {}
};

MODULE_DEVICE_TABLE(i2c, indev_id);

static const struct of_device_id indev_of_match[] = {
    {
        .compatible = "my_input_device"
    },
    {}
};

MODULE_DEVICE_TABLE(of, indev_of_match);

static struct i2c_driver indev_i2c_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "my_input_device",
        .of_match_table = indev_of_match
    },
    .probe = indev_probe,
    .remove = indev_remove,
    .id_table = indev_id
};

module_i2c_driver(indev_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C Polled Input Device.");
MODULE_AUTHOR("Jorge Miranda");