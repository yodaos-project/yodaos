###LED硬件层工作流程
####LED HAL参数配置
----
- LED驱动属性分类
	make menuconfig -> rokid -> Hardware Layer Solutions -> 
	这里有三种接口的led驱动：PWM格式，MCU侧I2C，ARM侧I2C(以kamino18为例)
- LED灯的数量
	make menuconfig -> rokid -> Hardware Layer Solutions ->
	由`BOARD_LED_NUMS`可以控制led灯的个数


####LED HAL代码逻辑
- 标准的android hardware layer架构,代码目录：`kamino18/hardware/modules/leds/`
- HAL层代码基本格式
       static struct hw_module_methods_t led_module_methods = {
           .open = led_dev_open,
       };

       struct hw_module_t HAL_MODULE_INFO_SYM = {
           .tag = HARDWARE_MODULE_TAG,
           .module_api_version = LEDS_API_VERSION,
           .hal_api_version = HARDWARE_HAL_API_VERSION,
           .id = LED_ARRAY_HW_ID,
           .name = "ROKID PWM LEDS HAL",
           .methods = &led_module_methods,
       };
- HAL层的调用方法：
		hw_get_module(LED_ARRAT_HW_ID, (const struct hw_module_t **)&module);
- 处理应用层发过RGB数据
	第一入口函数`led_draw()`，主要实现以下功能：
	+ 如果是RGB格式的三色灯，直接按照对应顺序刷写进底层驱动
	+ 如果是PWM控制的单色灯，会响应把RGB数据通过`rgb_to_gray()`转换成灰度值发到驱动层
	+ 以上各色灯自动丢弃透明度值

