#include "OA.h"

//����״�̽�⵽ǰ���ϰ����������
u16 obstacle_array[20];
u8 obstacle_array_bitmask[20];
u8 deducted_obstacle_array[5];

//���ñ�����������
u8 ZONE_FLAG =HOT_ZONE;

u16 raw_distance,processed_distance;
u8 initial_array_fill_flag = 0;
/*�趨��������*/

avoidance_obj lidar_avoid_params={35,150,25};

#if OA_SENSOR == Lidar
u8 obstacle_detection(avoidance_obj lidar_avoid_params,u8 sample_rate,float f_c,
											UART_HandleTypeDef *huart,RingBuff_t * ringbuff,u16 RINGBUFF_LEN,u8 *frame_data)
{
	int32_t angle  = 0;
	int32_t temp1=0,temp2=0;//��ʱ����
	u8 divide_span,skip_digits;
	u8 minimum_dis = 0;
	divide_span = (end_angle_pos - start_angle_pos)/5;
	skip_digits = (20-(end_angle_pos - start_angle_pos))/2;

	
	if(last_servo_pos != servo_pos)
	{
		last_servo_pos = servo_pos;	
		/*Step 1:����ϰ�����룬��ȡ��ǰ����Ƕ�*/
		read_one_frame(&raw_distance,huart,ringbuff,LENGTH_OF_BUFF,frame_data);
		//read_one_frame(&raw_distance,&huart2,&RINGBUFF_UART2,LENGTH_OF_BUFF,frame_data);
		if(raw_distance < 2)
		{
			HAL_Delay(20);
			read_one_frame(&raw_distance,huart,ringbuff,LENGTH_OF_BUFF,frame_data);
		}
		
		processed_distance = raw_distance;//lidar_data_process(raw_distance,processed_distance,sample_rate,f_c);//
		angle = convert_servo_angle(servo_pos);
		/*Step 2:��ȡ�ϰ������ݣ��ȴ��ϰ���������*/
		obstacle_array[servo_pos-5] = processed_distance;
		//printf("Angle:%d ,front distance:%d \n",angle,raw_distance);
		
		/*�����Ѿ��ռ���һ���ϰ������ݺ��ٿ�ʼ���ݴ���*/
		if(initial_array_fill_flag < (20-2*skip_digits))
		{
			initial_array_fill_flag += 1;
		}else 
		{
			HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);//���� PWM ͨ��
			HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3);//���� PWM ͨ��
			initial_array_fill_flag = 0xFF;
		}
		/*Step 3:�����ϰ�����*/
		if(initial_array_fill_flag == 0xFF)
		{
			//�ҳ��ϰ������С����
			minimum_dis = obstacle_array[skip_digits];
			for(temp1=(skip_digits);temp1<20-(skip_digits);temp1++)
			{
				if(obstacle_array[temp1]<=minimum_dis)
					minimum_dis = obstacle_array[temp1];
			}
			
			/*Sub step 1:�����ϰ����ƽ�����룬���С������hotzone�趨�ķֽ�ֵ�����趨Ϊ����ģʽ*/
			temp2 = 0;
			for(temp1=(skip_digits);temp1<20-(skip_digits);temp1++)
			{
				temp2 += obstacle_array[temp1];
			}
			temp2 /= 20;
			if(temp2 <= lidar_avoid_params.hotzone_limit) ZONE_FLAG = HOT_ZONE;
			else ZONE_FLAG = COLD_ZONE;
			//printf("Current mode:%d\n",ZONE_FLAG);
			
			/*Sub step 2:��������20���ϰ�������*/
			if(processed_distance >= lidar_avoid_params.secure_distance)
			{
				obstacle_array_bitmask[servo_pos-5] = 0;
			}else
			{
				obstacle_array_bitmask[servo_pos-5] = 1;//1��ʾ��⵽�ϰ����ڱ��Ϸ�Χ��
			}
			
			/*Sub step 2:�ѳ���Ϊ20���ϰ�����������Ϊ5*/
			deducted_obstacle_array[0] = 0;
			deducted_obstacle_array[4] = 0;
			for(temp2=0;temp2<divide_span;temp2++)
			{
				if(obstacle_array_bitmask[(skip_digits)+temp2]==1)deducted_obstacle_array[0] = 1;
				if(obstacle_array_bitmask[(skip_digits+4*divide_span)+temp2]==1)deducted_obstacle_array[4] = 1;
			}
			
//������			
//			printf("\n");			
//			for(temp1=0;temp1<20;temp1++)
//			{
//				printf("%d ",obstacle_array[temp1]);
//			}
//			printf("\n");
			
			for(temp1=1;temp1<4;temp1++)
			{
				deducted_obstacle_array[temp1] = 0;
				for(temp2=0;temp2<divide_span;temp2++)
				{
					deducted_obstacle_array[temp1] += obstacle_array_bitmask[(skip_digits+temp1*divide_span)+temp2];
				}
				deducted_obstacle_array[temp1] = (deducted_obstacle_array[temp1]/(divide_span/2))>1?1:0;
			}

		}
		return minimum_dis;		
	}
	else return 0;
}
#endif


void obstacle_avoidance(avoidance_obj lidar_avoid_params,u8 * deducted_obstacle_array,u8 minimumm_distance)
{
	u16 byte_form_array=0;
	u8 temp1;
	u16 temp2;
//	printf("minimum distance:%d, set param:%d",minimumm_distance,lidar_avoid_params.minimum_distance);
//	printf("\n");
//	for(temp1=0;temp1<5;temp1++)
//	{
//		printf("%d ",deducted_obstacle_array[temp1]);
//	}printf("\n");
	
	for(temp1=0;temp1<5;temp1++)
	{
		temp2 = deducted_obstacle_array[temp1];
		byte_form_array |= temp2 << (4-temp1);
	}
	//printf("byte form:%d\n",byte_form_array);
	
	if(byte_form_array == 0x17||byte_form_array == 0x15||
		byte_form_array == 0x1d||minimumm_distance <= lidar_avoid_params.minimum_distance)backward(16);//��1�ĸ�������4���ϰ�����Ϊ10101ʱ������
	else if((byte_form_array & 0x0e) == 0x00) forward(16);
	else if((byte_form_array&0x1c)>>3 >= (byte_form_array&0x07)) left_steel();
	else if((byte_form_array&0x1c)>>3 < (byte_form_array&0x07)) right_steel();
	else printf("undefined");
	
	return;
}