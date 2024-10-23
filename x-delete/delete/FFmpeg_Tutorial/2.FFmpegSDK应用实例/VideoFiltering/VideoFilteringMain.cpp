#include "common.h"

/*************************************************
	Function:		hello
	Description:	���������д���Ĳ���
	Calls:			��
	Called By:		main
	Input:			(in)argc : Ĭ�������в���
					(in)argv : Ĭ�������в���					
	Output:			(out)io_param : ���������еĽ��
	Return:			true : �����н�����ȷ
					false : �����н�������
*************************************************/
static int hello(int argc, char **argv, IOFiles &files)
{
	if (argc < 4) 
	{
		printf("usage: %s output_file input_file filter_index\n"
			"Filter index:.\n"
			"1. Color component\n"
			"2. Blur\n"
			"3. Horizonal flip\n"
			"4. HUE\n"
			"5. Crop\n"
			"6. Box\n"
			"7. Text\n"
			"\n", argv[0]);

		return -1;
	}

	files.inputFileName = argv[1];
	files.outputFileName = argv[2];
	files.frameWidth = atoi(argv[3]);
	files.frameHeight = atoi(argv[4]);
	files.filterIdx = atoi(argv[5]);

	fopen_s(&files.iFile, files.inputFileName, "rb+");
	if (!files.iFile)
	{
		printf("Error: open input file failed.\n");
		return -1;
	}

	fopen_s(&files.oFile, files.outputFileName, "wb+");
	if (!files.oFile)
	{
		printf("Error: open output file failed.\n");
		return -1;
	}

	return 0;
}

/*************************************************
	Function:		setFilterDescr
	Description:	������������Filter������
	Calls:			��
	Called By:		main
	Input:			(in)idx : Filter������
	Output:			(out)filter_descr : Filter������
	Return:			��
*************************************************/
static void setFilterDescr(int idx, char **filter_descr)
{
	switch (idx)
	{
	case 1:
		*filter_descr = "lutyuv='u=128:v=128'";
	case 2:
		*filter_descr = "boxblur";
	case 3:
		*filter_descr = "hflip";
	case 4:
		*filter_descr = "hue='h=60:s=-3'";
	case 5:
		*filter_descr = "crop=2/3*in_w:2/3*in_h";
	case 6:
		*filter_descr = "drawbox=x=100:y=100:w=100:h=100:color=pink@0.5";
	case 7:
		*filter_descr = "drawtext=fontfile=arial.ttf:fontcolor=green:fontsize=30:text='FFMpeg Filter Demo'";
	}
}

/*************************************************
Function:		main
Description:	��ڵ㺯��
*************************************************/
int main(int argc, char **argv)
{
	int ret = 0;
	AVFrame *frame_in = NULL;  
	AVFrame *frame_out = NULL;  
	unsigned char *frame_buffer_in = NULL;  
	unsigned char *frame_buffer_out = NULL;  
	char *filter_descr = NULL;

	IOFiles files = {NULL};

	//�����н���
	if (hello(argc, argv, files) < 0)
	{
		goto end;
	}

	int frameWidth = files.frameWidth, frameHeight = files.frameHeight;

	//����Filter������
	setFilterDescr(files.filterIdx, &filter_descr);

	avfilter_register_all();
	
	//��ʼ��Filter��ؽṹ
	if (ret = Init_video_filter(filter_descr, frameWidth, frameHeight))
	{
		goto end;
	}

	//������������frame����洢�ռ�
	Init_video_frame_in_out(&frame_in, &frame_out, &frame_buffer_in, &frame_buffer_out, frameWidth, frameHeight);

	while (Read_yuv_data_to_buf(frame_buffer_in, files, &frame_in)) 
	{
		//������frame��ӵ�filter graph
		if (!Add_frame_to_filter(frame_in))
		{
			printf("Error while adding frame.\n");
			goto end;
		}

		//��filter graph�л�ȡ���frame
		if (!Get_frame_from_filter(&frame_out))
		{
			printf("Error while getting frame.\n");
			goto end;
		}

		//�����frameд��������ļ�
		Write_yuv_to_outfile(frame_out, files);

		printf("Process 1 frame!\n");  
		av_frame_unref(frame_out);  
	}

end:
	
	//�ر��ļ�����ؽṹ
	fclose(files.iFile);
	fclose(files.oFile);

	av_frame_free(&frame_in);
	av_frame_free(&frame_out);

	Close_video_filter();

	return 0;
}