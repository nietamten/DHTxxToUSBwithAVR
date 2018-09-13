#include "stdafx.h"
#include "ctime"
#include "string"
#include "list"
#include "sstream"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/Fl_input.h>
#include <FL/Fl_button.h>
#include <FL/Fl_ask.h>
#include <FL/Fl_Check_Button.H>
#include "hidapi.h"

std::wstring fileName;
int maxPoints = -1;
int skipPoints = -1;
bool readOnly;

HANDLE m_hCommPort = NULL;
HANDLE dataFile = NULL;

std::list<hid_device*> devLst;

struct ChartPoint 
{
	INT16 dev,hr,t;
	time_t time;
};

class Fl_ChartWindow :public Fl_Double_Window
{
public:

	std::list<ChartPoint> data;

	void Fl_ChartWindow::draw()
	{
		Fl_Window::draw();

		fl_color(fl_rgb_color(0, 0, 0));
		
		int move = h() / 15;

		int Ytop = move;
		int Ybottom = move * 15;
		int Yzero = move * 10;

		for(int i=1;i<16;i++)
		{
			char str[5];
			if (i < 12)
				itoa((11-i)*10, str, 10);
			else
				itoa(-((i-11)*10), str, 10);
			fl_draw(str,0, i*move);
			fl_line(0, i*move, w(), i*move);
		}
		
		move = w() / 10;

		if (!data.empty())
		{
			time_t start = data.begin()->time;
			time_t end = data.rbegin()->time;
			
			for (int i = 0;i < 10;i++)
			{
				time_t time = start + ((end - start) / 10)*i;
				tm *ltm = localtime(&time);
				
				char tmp[5];
				std::string str1, str2;

				itoa(1900 + ltm->tm_year, tmp, 10);
				str1 += tmp;
				str1 += "-";
				itoa(1 + ltm->tm_mon, tmp, 10);
				str1 += tmp;
				str1 += "-";
				itoa(ltm->tm_mday, tmp, 10);
				str1 += tmp;

				itoa(1 + ltm->tm_hour, tmp, 10);
				str2 += tmp;
				str2 += ":";
				itoa(1 + ltm->tm_min, tmp, 10);
				str2 += tmp;
				str2 += ":";
				itoa(1 + ltm->tm_sec, tmp, 10);
				str2 += tmp;

				fl_draw(str1.c_str(), i*move, fl_height());
				fl_draw(str2.c_str(), i*move, 2 * fl_height());
				fl_line(i*move, 0, i*move, h());
			}
		
			for (int k = 0;k < 2;k++)
			{
				int diff = end - start;
				int lastX = -1, lastYT, lastYHR;
				int j = 0;

				for (std::list<ChartPoint>::iterator i = data.begin();
					i != data.end();
					i++)
				{
					if (i->dev == k)
					{
						int pDiff = i->time - start;
						int x;
						if (maxPoints > 0)
							x = w() - w() / maxPoints*j++;
						else
							x = ((double)pDiff / (double)diff)*(double)w();

						int yT;
						if (i->t > 0)
							yT = Ytop + Yzero - (((double)i->t / (double)10000) * (double)(Yzero));
						else
							yT = Yzero + (((double)-i->t / (double)4000) * (double)(Ybottom - Yzero));

						int yHR = Ytop + Yzero - (((double)i->hr / (double)10000) * (double)(Yzero));

						if (lastX != -1)
						{
							if (i->dev == 0)
							{
								fl_line_style(FL_SOLID, 1, 0);
								fl_color(fl_rgb_color(200, 0, 0));
								fl_line(lastX, lastYT, x, yT);
								fl_line_style(FL_SOLID, 2, 0);
								fl_color(fl_rgb_color(0, 0, 200));
								fl_line(lastX, lastYHR, x, yHR);


							}
							else
							{
								fl_line_style(FL_DASH, 1, 0);
								fl_color(fl_rgb_color(255, 0, 0));
								fl_line(lastX, lastYT, x, yT);
								fl_line_style(FL_DASH, 2, 0);
								fl_color(fl_rgb_color(0, 0, 255));
								fl_line(lastX, lastYHR, x, yHR);
							}

						}

						lastX = x;
						lastYT = yT;
						lastYHR = yHR;
					}
				}
			}
		}
	}

	Fl_ChartWindow::Fl_ChartWindow(int a, int b)
		:Fl_Double_Window(a, b)
	{

	}
};

Fl_ChartWindow *window;

void	Idle(void *data)
{
	HANDLE file = m_hCommPort;
	if (data != NULL)
		file = dataFile;
	
	std::string  res;
	DWORD dwBytesRead, dwBytesWritten;
	OVERLAPPED ov;
	char szBuf[100];
	if (data != NULL)
	{
		int skipped = 0;
		do
		{
			ChartPoint cp;
			ReadFile(file, &cp, sizeof(ChartPoint), &dwBytesRead, NULL);

			if (skipPoints > 0 && skipped < skipPoints)
			{
				skipped++;
				continue;
			}
			else if (dwBytesRead!=0)
			{
				window->data.push_back(cp);
			}
		} while (dwBytesRead > 0);
	}
	else
	{
		int num = 0;
		for (std::list<hid_device*>::iterator i = devLst.begin();
			i != devLst.end();
			i++)
		{
			UINT8 toSet[] = { 0x00,0x00,0x00,0x00 };
			UINT8 res = hid_read(*i, toSet, sizeof(toSet));
			if (res == 4)
			{
				INT16 t = toSet[0] | (toSet[1] << 8);
				INT16 h = toSet[2] | (toSet[3] << 8);
				
				ChartPoint cp;
				cp.time = time(0);
				cp.dev = num++;
				cp.hr = h;
				cp.t = t;
				if (maxPoints > 0 && window->data.size() > maxPoints)
					window->data.pop_front();
				window->data.push_back(cp);
				if (dataFile)
					WriteFile(dataFile, &cp, sizeof(ChartPoint), &dwBytesWritten, NULL);
			}
		}
	}
	window->redraw();
}

void system_error(char *name) {
	wchar_t *ptr = NULL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		0,
		GetLastError(),
		0,
		ptr,
		1024,
		NULL);

	fprintf(stderr, "\nError %s: %s\n", name, ptr);
	fl_alert("\nError %s: %s\n", name, ptr);
	LocalFree(ptr);
}

Fl_Window* w;
Fl_Check_Button* o1;
Fl_Input* o2;
Fl_Input* o3;
Fl_Input* o4;

void button_cb(Fl_Widget* obj, void*)
{	
	if (strlen(o2->value()) > 0)
	{
		int cSize = strlen(o2->value());
		std::wstring wc(cSize, L'#');
		fileName.resize(cSize);
		mbstowcs(&fileName[0], o2->value(), cSize);
	}
	readOnly = o1->value();
	if (strlen(o3->value()) > 0)
		maxPoints = atoi(o3->value());
	if (strlen(o4->value()) > 0)
		skipPoints = atoi(o4->value());

	if (!readOnly)
	{
		if (hid_init())
			exit(-1);

		struct hid_device_info *devs, *cur_dev;
		
		devs = hid_enumerate(0x0, 0x0);
		cur_dev = devs;

		UINT8 toSet[] = { 0x00,0x00,0x00,0x00 };

		*((UINT16*)&toSet[1]) = 2000;

		while (cur_dev) {
			if (cur_dev->vendor_id == 0x16c0 &&
				cur_dev->product_id == 0x05df)
			{
				hid_device *h = hid_open_path(cur_dev->path);

				if (0 > hid_send_feature_report(h, toSet, sizeof(toSet))) {
					exit(-2);
				}

				//hid_set_nonblocking(h, 1);

				devLst.push_back(h);
			}
			cur_dev = cur_dev->next;
		}
		hid_free_enumeration(devs);
		/*


		memset(toSet, 0, sizeof(toSet));
		//while(1)
		if (hid_get_feature_report(handle, toSet, sizeof(toSet)) < 0) {
		printf("Unable to get a feature report.\n");
		printf("%ls", hid_error(handle));
		}
		else
		{
		static int i = 0;
		printf(" feature report OK%d\n", i++);Sleep(500);
		}

		res = -1;
		int z = 0;

		while (1) {
		res = hid_read(handle, toSet, sizeof(toSet));
		if (res == 0)
		{
		//printf("waiting...\n");
		}
		if (res < 0)
		printf("Unable to read()\n");
		else if (res == 4)
		{
		INT16 t = toSet[0] | (toSet[1] << 8);
		INT16 h = toSet[2] | (toSet[3] << 8);
		//			INT16 t = toSet[1];
		//			INT16 h = toSet[3];

		printf("%d\t t = %d h = %d \n", z++, t, h);
		res = -1;
		}
		else
		{
		res = -1;
		}
		#ifdef WIN32
		Sleep(50);
		#else
		usleep(500 * 1000);
		#endif
		if (hid_get_feature_report(handle, toSet, sizeof(toSet)) < 0) {
		printf("Unable to get a feature report.\n");
		printf("%ls", hid_error(handle));
		}
		}

		hid_close(handle);


		hid_exit();
		*/
	}


	if (!fileName.empty())
	{
		dataFile = CreateFile(fileName.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			CREATE_NEW,
			0,
			NULL);
		
		if (INVALID_HANDLE_VALUE == dataFile) {
			dataFile = CreateFile(fileName.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			if (INVALID_HANDLE_VALUE == dataFile)
			system_error("opening file");

		}

		Idle((void*)1);
	}

	Fl::add_idle(Idle);
	
	w->hide();

	window->size_range(100,100);
	window->show(0, 0);
}

struct hid_device_info *devs, *cur_dev;

int main(int argc, char **argv) {

	window = new Fl_ChartWindow(640, 480);
	
	{ Fl_Window* o = new Fl_Window(541, 168);
	w = o; if (w) {/* empty */ }
	{  o1 = new Fl_Check_Button(265, 5, 275, 25, "tylko odczyt pliku");
	o1->down_box(FL_DOWN_BOX);
	} // Fl_Check_Button* o
	{  o2 = new Fl_Input(265, 30, 275, 25, "Nazwa pliku");
	} // Fl_Input* o
	{  o3 = new Fl_Input(265, 55, 275, 25, "Maksymalna ilo\305\233\304\207 wy\305\233wietlanych punkt\303\263w");
	} // Fl_Input* o
	{  o4 = new Fl_Input(265, 80, 275, 25, "Ilo\305\233\304\207 punkt\303\263w do pomini\304\231""cia");
	} // Fl_Input* o
	{ Fl_Button* o = new Fl_Button(140, 125, 190, 25, "Dalej");
	o->callback(button_cb);
	} // 
	o->end();
	} // Fl_Double_Window* o
	w->show(0,0);

	Fl::run(); 

	CloseHandle(m_hCommPort);
	CloseHandle(dataFile);

	return 0;
}