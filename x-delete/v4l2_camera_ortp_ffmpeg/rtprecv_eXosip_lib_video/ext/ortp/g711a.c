#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#if 1
#define         SIGN_BIT        (0x80)      /* Sign bit for a A-law byte. */
#define         QUANT_MASK      (0xf)       /* Quantization field mask. */
#define         NSEGS           (8)         /* Number of A-law segments. */
#define         SEG_SHIFT       (4)         /* Left shift for segment number. */
#define         SEG_MASK        (0x70)      /* Segment field mask. */
#define         BIAS            (0x84)      /* Bias for linear code. */
#define         CLIP            8159
 
#define         G711_A_LAW      (0)
#define         G711_MU_LAW     (1)
#define         DATA_LEN        (16)

uint8_t g711a_encode(int16_t pcm_val);
int16_t g711a_decode(uint8_t a_val);

static int16_t seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};

static uint8_t search(int16_t value)
{
    int i;

    for (i = 0; i < 8; i++) {
		if (value <= seg_aend[i]) {
			return i;
		}
	}
	
    return i;
}

uint8_t g711a_encode(int16_t pcm_val)
{
	int mask;
	uint8_t seg; 
	unsigned char aval;

	pcm_val = pcm_val >> 3;

	if (pcm_val >= 0) {
		/* sign (7th) bit = 1 */
		mask = 0xD5;
	} else {
		/* sign bit = 0 */
		mask = 0x55;
		pcm_val = -pcm_val - 1;
	}

	seg = search(pcm_val);

	/* out of range, return maximum value. */
	if (seg >= 8) {
		return (uint8_t) (0x7F ^ mask);
	} else { 
		aval = (uint8_t) seg << SEG_SHIFT;

		if (seg < 2) {
			aval |= (pcm_val >> 1) & QUANT_MASK;
		} else {
			aval |= (pcm_val >> seg) & QUANT_MASK;
		}
		
		return (aval ^ mask);
	}
}


int16_t g711a_decode(uint8_t a_val)
{
	int t;
	int seg;

	a_val ^= 0x55;
	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	
	switch (seg) {
	case 0:
    	t += 8;
		break;     
    case 1:
    	t += 0x108;
       	break;
    default:
		t += 0x108;
		t <<= seg - 1;
		break;
	}

	return ((a_val & SIGN_BIT) ? t : -t);
}


int main_xxxx(void) {
    int16_t i, sample = 1000;
    
    uint8_t encoded = g711a_encode(sample);
    int16_t decoded = g711a_decode(encoded);

    printf("原始样本: %d\n", sample);
    printf("编码后的样本: %d (0x%02x)\n", encoded, encoded);
    printf("解码后的样本: %d\n", decoded);
    

    for (sample = 0; sample < 5000; sample++) {
    	encoded = g711a_encode(sample);
    	decoded = g711a_decode(encoded);
    	printf("sample = %d, encoded = %d, 0x%02x, decoded = %d, 0x%04x\n", sample, encoded, encoded, decoded, decoded);
    }

    return 0;
}

#endif















#if 0
#define         SIGN_BIT        (0x80)      /* Sign bit for a A-law byte. */
#define         QUANT_MASK      (0xf)       /* Quantization field mask. */
#define         NSEGS           (8)         /* Number of A-law segments. */
#define         SEG_SHIFT       (4)         /* Left shift for segment number. */
#define         SEG_MASK        (0x70)      /* Segment field mask. */
#define         BIAS            (0x84)      /* Bias for linear code. */
#define         CLIP            8159
 
#define         G711_A_LAW      (0)
#define         G711_MU_LAW     (1)
#define         DATA_LEN        (16)

//强度表
static short seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};
//寻找当前的强度位，其实就是前八位的匹配
/*前八位可能性
0x0000 0000
0x0000 0001      
0x0000 0010   
0x0000 0100  
0x0000 1000
0x0001 0000
0x0010 0000
0x0100 0000
*/

static int search(int val, short *table, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        if (val <= *table++)
            return i;
    }
    return size;
}


uint8_t g711a_encode(int16_t value)
{
	return 0;
}

int16_t g711a_decode(uint8_t value)
{
	return 0;
}

// 编码函数
unsigned char linear2alaw_123(int pcm_val)
{
	int mask;
	int seg; 
	unsigned char aval;

	pcm_val = pcm_val >> 3;

	if (pcm_val >= 0) {
		/* sign (7th) bit = 1 */
		mask = 0xD5;
	} else {
		/* sign bit = 0 */
		mask = 0x55;
		pcm_val = -pcm_val - 1;
	}

	seg = search(pcm_val, seg_aend, 8);

	/* out of range, return maximum value. */
	if (seg >= 8) {
		return (unsigned char) (0x7F ^ mask);
	} else { 
		aval = (unsigned char) seg << SEG_SHIFT;

		if (seg < 2) {
			aval |= (pcm_val >> 1) & QUANT_MASK;
		} else {
			aval |= (pcm_val >> seg) & QUANT_MASK;
		}
		
		return (aval ^ mask);
	}
}

int alaw2linear_123(unsigned char a_val)
{
	int t;
	int seg;

	a_val ^= 0x55;
	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	
	switch (seg) {
	case 0:
    	t += 8;
		break;     
    case 1:
    	t += 0x108;
       	break;
    default:
		t += 0x108;
		t <<= seg - 1;
		break;
	}

	return ((a_val & SIGN_BIT) ? t : -t);
}




// 编码函数
unsigned char linear2alaw(int pcm_val)
{
int mask;
int seg; 
unsigned char aval;
// 移除低位3bit
pcm_val = pcm_val >> 3;
// 根据值正负确定mask掩码
if (pcm_val >= 0) {
mask = 0xD5;/* sign (7th) bit = 1 */
} else {
mask = 0x55;/* sign bit = 0 */
// 因为十三折线输入信号正负范围都是一样的，但是有符号的类型如char是-128~+127
// 负数是多一位，这里减一刚好和正数对应，并且pcm_val变为正值了，对应去编码
pcm_val = -pcm_val - 1;
}

// 查找序号位index
seg = search(pcm_val, seg_aend, 8);

//	编码超过最大值，去范围内最大值0x7f
if (seg >= 8)/* out of range, return maximum value. */
return (unsigned char) (0x7F ^ mask);
else { 
aval = (unsigned char) seg << SEG_SHIFT;
// 为什么要分两种情况？seg为2说明序号位位于第一位，再他后面只有5个bit了，只能移动一位
if (seg < 2) 
aval |= (pcm_val >> 1) & QUANT_MASK;
// seg大于2情况右移seg个bit，只保留其后4位强度位即可
else 
aval |= (pcm_val >> seg) & QUANT_MASK;
// 异或处理返回
return (aval ^ mask);
}
}

int alaw2linear(unsigned char a_val)
{
        int t;
        int seg;
        //得到偶数位异或后的值
        a_val ^= 0x55;
        //获取强度位4bit，并且左移动4位，强度位也已经在编码前的位置了
        t = (a_val & QUANT_MASK) << 4;
        //获取序号位，也就是符号位后面第几位是1
        seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
        switch (seg) {
        //符号位全为0的情况，说明编码之前的值很小，而t是强度位保留下来了，就补偿加8，8刚好是一个字节低4位的最大值，当做补偿
        case 0:
                t += 8;
                break;
        //符号位为1，说明7位序号位刚好最后一位是1 也就是0x0100 中间强度位刚好4个字节为t 低4位补偿8      
        case 1:
                printf("i'm here");
                t += 0x108;
                break;
        default:
                //其他符号位则以序号位为1的情况进行左移动，将序号位移动到正确的位置上去
                t += 0x108;
                t <<= seg - 1;
 
        }
        //根据符号位确定返回正负值
        return ((a_val & SIGN_BIT) ? t : -t);
}






int main() {
    // 测试用例
    int16_t sample = 1000;
    
    int8_t encoded = linear2alaw(sample);
    int16_t decoded = alaw2linear(encoded);

    printf("原始样本: %d\n", sample);
    printf("编码后的样本: %d\n", encoded);
    printf("解码后的样本: %d\n", decoded);

    return 0;
}
#endif






/*

//#define MAX_PCM_S16 32767
#define MAX_PCM_S16 32635

short g711DecodeByte(unsigned char alaw)
{
    alaw ^= 0xD5;
    int sign = alaw & 0x80;
    int exponent = (alaw & 0x70) >> 4;
    int data = alaw & 0x0f;
    data <<= 4;
    data += 8;
    if( exponent != 0 )
        data += 0x100;
    if( exponent > 1 )
        data <<= (exponent - 1);

    return (short) (sign == 0 ? data : -data);
}

unsigned char g711EncodeByte(short pcm)
{
    int sign = (pcm & 0x8000) >> 8;
    if( sign != 0 )
        pcm = -pcm;
    if( pcm > MAX_PCM_S16 )
        pcm = MAX_PCM_S16;
    int exponent = 7;
    int expMask;
    for(expMask = 0x4000; (pcm & expMask) == 0 && exponent > 0; exponent--, expMask >>= 1){
    }
    int mantissa = (pcm >> ((exponent == 0) ? 4 : (exponent + 3))) & 0x0f;
    unsigned char alaw = (unsigned char) (sign | exponent << 4 | mantissa);
    return (unsigned char) (alaw ^ 0xD5);
}


int main() {
    // 测试用例
    int16_t sample = 1000;
    int8_t encoded = g711EncodeByte(sample);
    int16_t decoded = g711DecodeByte(encoded);

    printf("原始样本: %d\n", sample);
    printf("编码后的样本: %d\n", encoded);
    printf("解码后的样本: %d\n", decoded);

    return 0;
}


*/



