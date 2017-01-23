#include "acc.h"
#include "fc.h"
#include "../common.h"

acc_data_t acc_data;


#define LPF_beta 		0.05		//low pass filter setting
#define max_hold_time	50	//0.5s	//hold time of last peak value ( 1 = 10ms )

struct acc_gui_filter_data_t
{
	float old_acc;
	uint8_t hold_time;
};
acc_gui_filter_data_t gui_widget_accel;

void acc_widget_filter()
{
	float new_accel = fc.acc.total;
	if(new_accel >= gui_widget_accel.old_acc)
	{
		gui_widget_accel.old_acc = new_accel;
		gui_widget_accel.hold_time = 0;
	}
	else if((new_accel < gui_widget_accel.old_acc) and ( gui_widget_accel.hold_time < max_hold_time ) )
	{
		gui_widget_accel.hold_time += 1;
	}
	else
	{
		gui_widget_accel.old_acc = (gui_widget_accel.old_acc - (LPF_beta * (gui_widget_accel.old_acc - new_accel)));
	}
	fc.acc.total_widget_filtered = gui_widget_accel.old_acc;
}


void acc_save_calibration(vector_float_t sens_vf, vector_float_t bias_vf)
{
	vector_i16_t bias;
	vector_i16_t sens;

	sens.x = int16_t(sens_vf.x * 2);
	sens.y = int16_t(sens_vf.y * 2);
	sens.z = int16_t(sens_vf.z * 2);
	bias.x = int16_t(bias_vf.x * 2);
	bias.y = int16_t(bias_vf.y * 2);
	bias.z = int16_t(bias_vf.z * 2);

	eeprom_busy_wait();
	eeprom_update_block(&bias, &config_ro.calibration.acc_bias, sizeof(config_ro.calibration.acc_bias));
	eeprom_update_block(&sens, &config_ro.calibration.acc_sensitivity, sizeof(config_ro.calibration.acc_sensitivity));

	//DEBUG("written float: bias %f %f %f sens %f %f %f\n", bias_vf.x, bias_vf.y, bias_vf.z, sens_vf.x, sens_vf.y, sens_vf.z);
	//DEBUG("written int: bias %d %d %d sens %d %d %d\n", bias.x, bias.y, bias.z, sens.x, sens.y, sens.z);
}

void acc_load_calibration(vector_float_t * sens_vf, vector_float_t * bias_vf)
{
	vector_i16_t bias;
	vector_i16_t sens;

	eeprom_busy_wait();
	eeprom_read_block(&bias, &config_ro.calibration.acc_bias, sizeof(config_ro.calibration.acc_bias));
	eeprom_read_block(&sens, &config_ro.calibration.acc_sensitivity, sizeof(config_ro.calibration.acc_sensitivity));

	sens_vf->x = (float(sens.x) / 2);
	sens_vf->y = (float(sens.y) / 2);
	sens_vf->z = (float(sens.z) / 2);
	bias_vf->x = (float(bias.x) / 2);
	bias_vf->y = (float(bias.y) / 2);
	bias_vf->z = (float(bias.z) / 2);

	//DEBUG("loaded float: bias %f %f %f sens %f %f %f\n", bias_vf->x, bias_vf->y, bias_vf->z, sens_vf->x, sens_vf->y, sens_vf->z);
	//DEBUG("loaded int: bias %d %d %d sens %d %d %d\n", bias.x, bias.y, bias.z, sens.x, sens.y, sens.z);
}

void acc_calc_init()
{
	fc.acc.total = 1.0;
	gui_widget_accel.hold_time = 0;
	acc_load_calibration( &acc_data.calibration.sens, &acc_data.calibration.bias );
}

void acc_calc_vector() //calculate real acceleration using calibration values
{
	fc.acc.vector.x = (float(fc.acc.raw.x) - acc_data.calibration.bias.x) / acc_data.calibration.sens.x;
	fc.acc.vector.y = (float(fc.acc.raw.y) - acc_data.calibration.bias.y) / acc_data.calibration.sens.y;
	fc.acc.vector.z = (float(fc.acc.raw.z) - acc_data.calibration.bias.z) / acc_data.calibration.sens.z;
}

void acc_calc_total()	//calculate total acceleration
{
	fc.acc.total = ( sqrt(fc.acc.vector.x * fc.acc.vector.x + fc.acc.vector.y * fc.acc.vector.y + fc.acc.vector.z * fc.acc.vector.z) );
}