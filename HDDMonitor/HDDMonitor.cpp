#include "framework.h"
#include "HDDMonitor.h"
#include <CommCtrl.h>
#include <ShlObj.h>
#include <winioctl.h>
#include <time.h>
#include <shellapi.h>

//ID таймера
#define IDT_TIMER 10
//Сообщение от всплывающего уведомления
#define WM_NOTIFYMENU WM_USER + 1
//Константы
#define MAX_LOADSTRING 100
#define LIST_1_SIZE 6
#define LIST_2_SIZE 2
#define LIST_3_SIZE 3
//Состояния здоровья диска
#define HEALTH_NORMAL 0
#define HEALTH_CAUTION 1
#define HEALTH_BAD 2
#define HEALTH_CRITICAL 3
//Период автоматической проверки
enum AutoCheckPeriod {
    Period30min = 0,
    Period1h,
    Period5h,
    Period12h,
    Period1d,
    Period3d,
    Period7d
};
//настройки
struct SETTINGS {
    BOOL Startup = TRUE;
    BOOL AutoCheck = TRUE;
    AutoCheckPeriod CheckPeriod = Period1d;
    BOOL MakeLog = TRUE; // не используется
    BOOL UserLogFolder = FALSE;  // не используется
    BOOL DisplayHEX = FALSE;
    WCHAR UserDefinedPath[MAX_PATH] = L".\\";  // не используется
    time_t LastCheckTime = time(NULL);
};
//Аттрибут SMART
struct SMART_ATTRIBUTE {
    BYTE Index;
    DWORD RAWValue;
    BYTE Value;
    BYTE Worst;
    DWORD Threshold;
};
//Свойства диска
struct DISK {
    BOOL IsValid;
    DWORD DisksCount;
    DWORD DiskNumber;
    BYTE DiskName[40];
    DWORDLONG Capacity;
    BYTE HealthStatus;
    BYTE SmartValuesCount;
    SMART_ATTRIBUTE DiskSMART[30];
    BYTE TempCurrent = 200;
    BYTE Firmware[16];
    BYTE SerialNumber[32];
    BYTE Vendor[8];
    DWORD BytesPerSector;
};


// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна
INT List1ColSize[LIST_1_SIZE] = { 30,220,70,70,70,80 }; // Константы для ширины колонок Listview
INT List2ColSize[LIST_2_SIZE] = { 90,125 };
INT List3ColSize[LIST_3_SIZE] = { 30,50,135 };
SETTINGS CurrentSettings; //текущие настройки программы
DISK* ListDisks;    // массив дисков
BYTE ChoosenDisk = 0;
UINT_PTR hTimer = 0;
BOOL HideDlg;
NOTIFYICONDATAW nIData; //иконка в трее


//объявления функций
INT_PTR CALLBACK    MainWnd(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Settings(HWND, UINT, WPARAM, LPARAM);

BOOL                SettingsLoad(SETTINGS*); //загрузка настроек из файла
BOOL                SettingsStore(SETTINGS*); //сохранение настроек в файл
DISK*               GetDisks();//получение списка накопителей в системе
BOOL                GetDiskSMART(HANDLE, DISK*); // получение SMART для определенного диска
void                GetDiskHealth(DISK*); // рассчет количества здоровья определенного диска
BOOL                OutDisksToList(DISK*, HWND); //вывод в ListView
BOOL                OutDiskInfoToList(DISK*, HWND);
BOOL                OutDiskSMARTToList(DISK*, HWND, SETTINGS&);
void                OutDiskHealthToEdit(DISK*, HWND);
void                AddToAutoStart(BOOL);// добавление в автозагрузку
void                TimerFunc(HWND, UINT, UINT_PTR, DWORD); // функция обработчик таймера


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    
    if (lstrcmpW(lpCmdLine, L"-h") == 0) // если тихий запуск
        HideDlg = TRUE;
    else
        HideDlg = FALSE;

    hInst = hInstance;
    
    SettingsLoad(&CurrentSettings); // загружаем настройки

    ListDisks = GetDisks(); // получаем список дисков
    if (ListDisks != nullptr)
    {
        if (CurrentSettings.AutoCheck == TRUE)// запускаем таймер автопроверки, если нужно
        {
            hTimer = SetTimer(NULL, IDT_TIMER, 18000, (TIMERPROC)TimerFunc);     
        }
        DialogBox(hInst, MAKEINTRESOURCE(IDD_MAINDLG), NULL, MainWnd); // отображаетм главное меню
        delete[] ListDisks;
    }

    AddToAutoStart(CurrentSettings.Startup);// добавляем в автозагрузку, если нужно

    SettingsStore(&CurrentSettings); // сохраняем настройки

    KillTimer(NULL, hTimer);
    hTimer = 0;
    
    return 0;
}


INT_PTR CALLBACK MainWnd(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        //создание и наполнение ListView
        SetClassLong(hDlg, GCL_HICONSM, (LONG)LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL)));
        LVCOLUMN lvc;
        WCHAR szColTitle[256];

        lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvc.pszText = szColTitle;   
        
        for (int i = 0; i < LIST_1_SIZE; i++)
        {
            lvc.iSubItem = i;
            lvc.cx = List1ColSize[i];
            lvc.fmt = LVCFMT_RIGHT;
            if (i == 1)
                lvc.fmt = LVCFMT_LEFT;

            LoadString(hInst, IDS_COLUMN_1 + i, szColTitle, 256);
            ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST1), i, &lvc);
        }

        lvc.fmt = LVCFMT_LEFT;

        for (int i = 0; i < LIST_2_SIZE; i++)
        {
            lvc.iSubItem = i;
            lvc.cx = List2ColSize[i];

            LoadString(hInst, IDS_COLUMN_2 + i, szColTitle, 256);
            ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST2), i, &lvc);
        }

        for (int i = 0; i < LIST_3_SIZE; i++)
        {
            lvc.iSubItem = i;
            lvc.cx = List3ColSize[i];

            LoadString(hInst, IDS_COLUMN_3 + i, szColTitle, 256);
            ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST3), i, &lvc);
        }

        ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_LIST1), LVS_EX_FULLROWSELECT);
        ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_LIST2), LVS_EX_FULLROWSELECT);
        ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_LIST3), LVS_EX_FULLROWSELECT);
        //вывод данных в ListView и в EditBox
        OutDisksToList(ListDisks, GetDlgItem(hDlg, IDC_LIST3));

        OutDiskInfoToList(&ListDisks[ChoosenDisk], GetDlgItem(hDlg, IDC_LIST2));

        OutDiskSMARTToList(&ListDisks[ChoosenDisk], GetDlgItem(hDlg, IDC_LIST1), CurrentSettings);

        if (ListDisks[ChoosenDisk].TempCurrent >= 200)
        {
            wsprintf(szColTitle, L"N/A");
        }
        else
        {
            wsprintf(szColTitle, L"%u°C", ListDisks[ChoosenDisk].TempCurrent);
        }
        SetWindowText(GetDlgItem(hDlg, IDC_EDIT2), szColTitle);

        OutDiskHealthToEdit(&ListDisks[ChoosenDisk], GetDlgItem(hDlg, IDC_EDIT1));

        return (INT_PTR)TRUE;
    case WM_PAINT:
        //сворачивание окна в трей, если надо
        if (HideDlg == TRUE)
        {
            nIData.cbSize = sizeof(NOTIFYICONDATAW);
            nIData.hWnd = hDlg;
            nIData.uID = NULL;
            nIData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
            nIData.uCallbackMessage = WM_NOTIFYMENU;
            nIData.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));

            Shell_NotifyIcon(NIM_ADD, &nIData);

            ShowWindow(hDlg, SW_HIDE);
            HideDlg = FALSE;
        }
        break;
    case WM_NOTIFYMENU:
        //обработка нажатий на иконку в трее
        switch (lParam)
        {
        case WM_RBUTTONUP:
            POINT pMouse;
            GetCursorPos(&pMouse);
            TrackPopupMenu(GetSubMenu(LoadMenu(hInst, MAKEINTRESOURCE(IDC_HDDMONITOR)), 0), TPM_LEFTBUTTON, pMouse.x, pMouse.y, 0, hDlg, NULL);
            break;
        case WM_LBUTTONUP:
            ShowWindow(hDlg, SW_SHOW);
            Shell_NotifyIcon(NIM_DELETE, &nIData);
            break;
        case NIN_BALLOONUSERCLICK:
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDM_SHOW, 0), 0);
            break;
        }
    case WM_COMMAND:
        // обработка нажатий на кнопки
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            HideDlg = TRUE;
            SendMessage(hDlg, WM_PAINT, 0, 0);
            return (INT_PTR)TRUE;
            break;
        case IDC_BUTTON1:
            delete[] ListDisks;
            ListDisks = GetDisks();
            break;
        case IDC_BUTTON2:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGS), hDlg, Settings);
            break;
        case IDC_BUTTON3:
            WCHAR Buff[10];
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST1));
            OutDiskSMARTToList(&ListDisks[ChoosenDisk], GetDlgItem(hDlg, IDC_LIST1),CurrentSettings);
            if (ListDisks[ChoosenDisk].TempCurrent >= 200)
            {
                wsprintf(Buff, L"N/A");
            }
            else
            {
                wsprintf(Buff, L"%u°C", ListDisks[ChoosenDisk].TempCurrent);
            }

            SetWindowText(GetDlgItem(hDlg, IDC_EDIT2), Buff);

            OutDiskHealthToEdit(&ListDisks[ChoosenDisk], GetDlgItem(hDlg, IDC_EDIT1));
            break;
        
        case IDM_SHOW:
            ShowWindow(hDlg, SW_SHOW);
            Shell_NotifyIcon(NIM_DELETE, &nIData);
            break;
        case IDM_EXIT:
            Shell_NotifyIcon(NIM_DELETE, &nIData);
            ShowWindow(hDlg, SW_SHOW);
            EndDialog(hDlg, LOWORD(wParam));
            break;
        }
    case WM_NOTIFY:
        //обработка одинарных и двойных нажатий на ListView
        switch (LOWORD(wParam))
        {
        case IDC_LIST1:
            if (((LPNMHDR)lParam)->code == NM_DBLCLK)
            {
                HINSTANCE err;
                err = ShellExecute(hDlg, L"open", L"https://www.cropel.com/library/smart-attribute-list.aspx", NULL, NULL, SW_SHOWMAXIMIZED);
                if ((INT_PTR)err <= 32)
                {
                    MessageBox(hDlg, L"Error while opening browser", L"Error", MB_OK);
                }
            }
            break;
        case IDC_LIST3:

            if (((LPNMHDR)lParam)->code == NM_CLICK)
            {
                ChoosenDisk = (BYTE)SendMessage(GetDlgItem(hDlg, IDC_LIST3), LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST2));
                OutDiskInfoToList(&ListDisks[ChoosenDisk], GetDlgItem(hDlg, IDC_LIST2));
            }
            else if (((LPNMHDR)lParam)->code == NM_DBLCLK)
            {
                ChoosenDisk = (BYTE)SendMessage(GetDlgItem(hDlg, IDC_LIST3), LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST2));
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST1));
                OutDiskInfoToList(&ListDisks[ChoosenDisk], GetDlgItem(hDlg, IDC_LIST2));
                OutDiskSMARTToList(&ListDisks[ChoosenDisk], GetDlgItem(hDlg, IDC_LIST1), CurrentSettings);
                WCHAR Buff[10];
                if (ListDisks[ChoosenDisk].TempCurrent >= 200)
                {
                    wsprintf(Buff, L"N/A");
                }
                else
                {
                    wsprintf(Buff, L"%u°C", ListDisks[ChoosenDisk].TempCurrent);
                }
                SetWindowText(GetDlgItem(hDlg, IDC_EDIT2), Buff);

                OutDiskHealthToEdit(&ListDisks[ChoosenDisk], GetDlgItem(hDlg, IDC_EDIT1));
            }
            break;
        }
        break;
    case WM_CTLCOLORSTATIC:
        //перекрашивание EditBox в зависимости от состояния диска
        if((HWND)lParam == GetDlgItem(hDlg, IDC_EDIT1))
        {
            switch (ListDisks[ChoosenDisk].HealthStatus)
            {
            case HEALTH_NORMAL:
                SetTextColor((HDC)wParam, RGB(0, 0, 0));
                SetBkColor((HDC)wParam, RGB(0, 255, 0));
                return (INT_PTR)CreateSolidBrush(RGB(0, 255, 0));
            case HEALTH_CAUTION:
                SetBkColor((HDC)wParam, RGB(255, 255, 0));
                return (INT_PTR)CreateSolidBrush(RGB(255, 255, 0));
            case HEALTH_BAD:
                SetTextColor((HDC)wParam, RGB(255, 255, 0));
                SetBkColor((HDC)wParam, RGB(255, 0, 0));
                return (INT_PTR)CreateSolidBrush(RGB(255, 0, 0));
            case HEALTH_CRITICAL:
                SetTextColor((HDC)wParam, RGB(255, 255, 255));
                SetBkColor((HDC)wParam, RGB(0, 0, 0));
                return (INT_PTR)CreateSolidBrush(RGB(0, 0, 0));
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        //инициализация элементов управления
        SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_SETCHECK, CurrentSettings.Startup, 0);
        SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_SETCHECK, CurrentSettings.AutoCheck, 0);

        EnableWindow(GetDlgItem(hDlg, IDC_PERIOD), SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0));
        EnableWindow(GetDlgItem(hDlg, IDC_COMBO1), SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0));

        SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)L"30 мин.");
        SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)L"1 ч.");
        SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)L"5 ч.");
        SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)L"12 ч.");
        SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)L"1 день");
        SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)L"3 дня");
        SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LPARAM)L"7 дней");

        SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, CurrentSettings.CheckPeriod, 0);

        SendMessage(GetDlgItem(hDlg, IDC_CHECK4), BM_SETCHECK, CurrentSettings.MakeLog, 0);
        SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_SETCHECK, CurrentSettings.UserLogFolder, 0);

        EnableWindow(GetDlgItem(hDlg, IDC_CHECK5), SendMessage(GetDlgItem(hDlg, IDC_CHECK4), BM_GETCHECK, 0, 0));
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_GETCHECK, 0, 0));
        EnableWindow(GetDlgItem(hDlg, IDC_PATH), SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_GETCHECK, 0, 0));
        EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_GETCHECK, 0, 0));

        if (SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_GETCHECK, 0, 0))
        {
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), CurrentSettings.UserDefinedPath);
        }

        SendMessage(GetDlgItem(hDlg, IDC_CHECK6), BM_SETCHECK, CurrentSettings.DisplayHEX, 0);
       
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            //записть установленных настроек в переменную
            CurrentSettings.Startup = SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_GETCHECK, 0, 0);
            CurrentSettings.AutoCheck = SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0);
            CurrentSettings.MakeLog = SendMessage(GetDlgItem(hDlg, IDC_CHECK4), BM_GETCHECK, 0, 0);
            CurrentSettings.UserLogFolder = SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_GETCHECK, 0, 0);
            if (CurrentSettings.UserLogFolder == TRUE)
            {
                WCHAR buff[256];
                GetWindowText(GetDlgItem(hDlg, IDC_EDIT1), buff, 256);
                wsprintf(CurrentSettings.UserDefinedPath, L"%s", buff);
            }
            CurrentSettings.DisplayHEX = SendMessage(GetDlgItem(hDlg, IDC_CHECK6), BM_GETCHECK, 0, 0);

            if (CurrentSettings.AutoCheck == TRUE && hTimer == 0)
            {
                hTimer = SetTimer(NULL, IDT_TIMER, 18000, (TIMERPROC)TimerFunc);
            }
            else if (CurrentSettings.AutoCheck == FALSE && hTimer != 0)
            {
                KillTimer(NULL, hTimer);
                hTimer = 0;
            }

            SendMessage(GetParent(hDlg), WM_COMMAND, MAKEWPARAM(IDC_BUTTON3, 0), 0);
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_CHECK2:
            //переключение активности некоторых окон в зависимости от установленных Checkbox
            EnableWindow(GetDlgItem(hDlg, IDC_PERIOD), SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0));
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO1), SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0));
            break;
        case IDC_CHECK4:
            EnableWindow(GetDlgItem(hDlg, IDC_CHECK5), SendMessage(GetDlgItem(hDlg, IDC_CHECK4), BM_GETCHECK, 0, 0));
            SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_SETCHECK, 0, 0);
        case IDC_CHECK5:
            EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_GETCHECK, 0, 0));
            EnableWindow(GetDlgItem(hDlg, IDC_PATH), SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_GETCHECK, 0, 0));
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), SendMessage(GetDlgItem(hDlg, IDC_CHECK5), BM_GETCHECK, 0, 0));
            break;
        case IDC_BUTTON1:
            //выбор пользовательской папки для логов
            //не реализовано
            BROWSEINFO bi;
            WCHAR buff[MAX_PATH];
            bi.hwndOwner = hDlg;
            bi.pidlRoot = NULL;
            bi.lpszTitle = L"Choose folder";
            bi.lpfn = NULL;
            bi.pszDisplayName = buff;
            bi.ulFlags = BIF_BROWSEFORCOMPUTER | BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN;
            LPCITEMIDLIST lpItemDList;
            lpItemDList = SHBrowseForFolder(&bi);

            SHGetPathFromIDList(lpItemDList, CurrentSettings.UserDefinedPath);
            SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), CurrentSettings.UserDefinedPath);
            break;
        }

        switch (HIWORD(wParam))
        {
            //обработка выбора в Combobox
        case CBN_SELCHANGE:
            CurrentSettings.CheckPeriod = AutoCheckPeriod((UINT)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0));
            break;
        }
        break;

    case WM_DESTROY:
        if (CurrentSettings.UserDefinedPath[0] == '\0')
            CurrentSettings.UserLogFolder = FALSE;
        break;
    }
    return (INT_PTR)FALSE;
}

BOOL SettingsLoad(SETTINGS* Settings) 
{
    WCHAR ExePath[MAX_PATH];
    WCHAR WorkDir[MAX_PATH];
    WCHAR DriveLetter[3];
    GetModuleFileNameW(NULL, ExePath, MAX_PATH);// получаем путь к исполняемому файлу
    _wsplitpath_s(ExePath,DriveLetter, 3, WorkDir, MAX_PATH, NULL, NULL, NULL, NULL);
    wsprintf(ExePath, L"%s%s\\Settings.ini", DriveLetter, WorkDir);// получаем путь к файлу настроек

    HANDLE SettingsFile=CreateFile(ExePath,GENERIC_READ,FILE_SHARE_READ,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
    DWORD BytesRead;
    SETTINGS TSettings;

    if (SettingsFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, L"Error while loading settings. Defaults loaded", L"Error", MB_OK);
        return FALSE;
    }
    
    if (ReadFile(SettingsFile, &TSettings, sizeof(SETTINGS), &BytesRead, NULL) == 0)//читаем настройки во временную переменную
    {
        MessageBox(NULL, L"Error, settings file corrupted. Defaults loaded", L"Error", MB_OK);
        CloseHandle(SettingsFile);
        return FALSE;
    }

    if (BytesRead != sizeof(SETTINGS))
    {
        CloseHandle(SettingsFile);
        DeleteFile(ExePath);
        return FALSE;
    }

    if(TSettings.MakeLog==TRUE)
        if (TSettings.UserLogFolder == FALSE)
            wsprintf(TSettings.UserDefinedPath, L".\\");

    memcpy(Settings, &TSettings, sizeof(SETTINGS));// сохраняем настройки в память
    CloseHandle(SettingsFile);
    return TRUE;
}

BOOL SettingsStore(SETTINGS* Settings)
{
    WCHAR ExePath[MAX_PATH];
    WCHAR WorkDir[MAX_PATH];
    WCHAR DriveLetter[3];
    GetModuleFileNameW(NULL, ExePath, MAX_PATH);
    _wsplitpath_s(ExePath, DriveLetter, 3, WorkDir, MAX_PATH, NULL, NULL, NULL, NULL);
    wsprintf(ExePath, L"%s%s\\Settings.ini", DriveLetter, WorkDir);

    HANDLE SettingsFile = CreateFile(ExePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD BytesWritten;

    if (SettingsFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, L"Error while storing settings", L"Error", MB_OK);
        return FALSE;
    }

    if (Settings->UserLogFolder == FALSE)
    {
        ZeroMemory(Settings->UserDefinedPath, sizeof(WCHAR) * MAX_PATH);
        if (Settings->MakeLog == TRUE)
        {
            wsprintf(Settings->UserDefinedPath, L".\\");
        }
    }

    if (WriteFile(SettingsFile, Settings, sizeof(SETTINGS), &BytesWritten, NULL) == 0)//записываем настройки в файл
    {
        MessageBox(NULL, L"Error while storing settings", L"Error", MB_OK);
        CloseHandle(SettingsFile);
        return FALSE;
    }
   
    CloseHandle(SettingsFile);
    return TRUE;
}

DISK* GetDisks()
{
    HKEY Reg;
    LSTATUS sRet;
    BOOL bRet;
    DWORD NOfDisks;
    DWORD BuffSize;
    HANDLE hCurDisk;
    DISK_GEOMETRY_EX CurDiskLen;
    ZeroMemory(&CurDiskLen, sizeof(CurDiskLen));
    DISK* TempDiskArray; 
    int startSpaceSizeInSerialNum=0;

    STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader = { 0 };
    STORAGE_PROPERTY_QUERY query;
    ZeroMemory(&query, sizeof(STORAGE_PROPERTY_QUERY));
    query.QueryType = PropertyStandardQuery;
    query.PropertyId = StorageDeviceProperty;

    WCHAR DrivePath[20];

    sRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\services\\Disk\\Enum", REG_OPTION_NON_VOLATILE, KEY_READ, &Reg);
    if (sRet != ERROR_SUCCESS)
    {
        MessageBox(NULL, L"Error while opening registry", L"Critical error!", MB_OK);
        return NULL;
    }

    BuffSize = sizeof(DWORD);
    sRet = RegQueryValueExW(Reg, L"Count", NULL, NULL, (BYTE*)&NOfDisks, &BuffSize);//читаем из реестра количество физических жестких дисков
    if (sRet != ERROR_SUCCESS)
    {
        MessageBox(NULL, L"Error while reading registry item", L"Critical error!", MB_OK);
        RegCloseKey(Reg);
        return NULL;
    }
    RegCloseKey(Reg);

    TempDiskArray = new DISK[NOfDisks];
    TempDiskArray[0].DisksCount = NOfDisks;
    //производим для каждого диска сбор сведений
    for (unsigned int i = 0; i < NOfDisks; i++)
    {
        wsprintf(DrivePath, L"\\\\.\\PhysicalDrive%i", i);
        hCurDisk = CreateFile(DrivePath, GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hCurDisk == INVALID_HANDLE_VALUE)
        {
            MessageBox(NULL, L"Error while opening physical drive", L"Critical error!", MB_OK);
            delete[] TempDiskArray;
            return NULL;
        }
        //получаем размер структуры
        bRet = DeviceIoControl(hCurDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(STORAGE_PROPERTY_QUERY), &storageDescriptorHeader, sizeof(STORAGE_DESCRIPTOR_HEADER), &BuffSize, NULL);
        if (bRet == 0)
        {
            MessageBox(NULL, L"Error while getting drive properties", L"Critical error!", MB_OK);
            CloseHandle(hCurDisk);
            delete[] TempDiskArray;
            return NULL;
        }
        
        //выделяем память
        DWORD dwOutBufferSize = storageDescriptorHeader.Size;
        BYTE* pOutBuff = new BYTE[dwOutBufferSize];
        ZeroMemory(pOutBuff, dwOutBufferSize);
        //получаем свойства диска
        bRet = DeviceIoControl(hCurDisk, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(STORAGE_PROPERTY_QUERY), pOutBuff, dwOutBufferSize, &BuffSize, NULL);
        if (bRet == 0)
        {
            MessageBox(NULL, L"Error while getting drive properties", L"Critical error!", MB_OK);
            CloseHandle(hCurDisk);
            delete[] pOutBuff;
            delete[] TempDiskArray;
            return NULL;
        }

        STORAGE_DEVICE_DESCRIPTOR* des = (STORAGE_DEVICE_DESCRIPTOR*)pOutBuff;

        if (des->RemovableMedia == TRUE || bRet == FALSE)
        {
            TempDiskArray[i].IsValid = FALSE;
            CloseHandle(hCurDisk);
            delete[] pOutBuff;
            continue;
        }
        //записываем информацию в память
        TempDiskArray[i].IsValid = TRUE;
        TempDiskArray[i].DiskNumber = i;
        memcpy(TempDiskArray[i].DiskName, pOutBuff + des->ProductIdOffset, sizeof(BYTE)*40);
        memcpy(TempDiskArray[i].Firmware, pOutBuff + des->ProductRevisionOffset, sizeof(BYTE) * 16);
        startSpaceSizeInSerialNum = strspn((char*)pOutBuff + des->SerialNumberOffset, " ");
        memcpy(TempDiskArray[i].SerialNumber, pOutBuff + des->SerialNumberOffset+ startSpaceSizeInSerialNum, sizeof(BYTE) * (32- startSpaceSizeInSerialNum));
        memcpy(TempDiskArray[i].Vendor, pOutBuff + des->VendorIdOffset, sizeof(BYTE) * 8);

        delete[] pOutBuff;
        //узнаем размеры диска
        bRet = DeviceIoControl(hCurDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &CurDiskLen, sizeof(DISK_GEOMETRY_EX), &BuffSize, NULL);
        if (bRet == 0)
        {
            MessageBox(NULL, L"Error while getting drive properties", L"Critical error!", MB_OK);
            CloseHandle(hCurDisk);
            delete[] TempDiskArray;
            return NULL;
        }

        TempDiskArray[i].Capacity = CurDiskLen.DiskSize.QuadPart;
        TempDiskArray[i].BytesPerSector = CurDiskLen.Geometry.BytesPerSector;

        GetDiskSMART(hCurDisk, &TempDiskArray[i]); // получаем SMART
        GetDiskHealth(&TempDiskArray[i]);// рассчитываем здоровье
        
        CloseHandle(hCurDisk);   
    }

    return TempDiskArray;
}

BOOL GetDiskSMART(HANDLE hDisk, DISK* Disk)
{
    SENDCMDINPARAMS stCIP = { 0 };
    DWORD dwRet = 0;
    BOOL bRet = FALSE;
    BYTE szAttributes[sizeof(SENDCMDOUTPARAMS) + READ_ATTRIBUTE_BUFFER_SIZE - 1];

    UCHAR ucT1;
    PBYTE pT1, pT3; PDWORD pT2;
    SMART_ATTRIBUTE* pSmartValues;
    // производим запрос SMART параметров диска
    stCIP.cBufferSize = READ_ATTRIBUTE_BUFFER_SIZE;
    stCIP.bDriveNumber = 0;
    stCIP.irDriveRegs.bFeaturesReg = READ_ATTRIBUTES;
    stCIP.irDriveRegs.bSectorCountReg = 1;
    stCIP.irDriveRegs.bSectorNumberReg = 1;
    stCIP.irDriveRegs.bCylLowReg = SMART_CYL_LOW;
    stCIP.irDriveRegs.bCylHighReg = SMART_CYL_HI;
    stCIP.irDriveRegs.bDriveHeadReg = 0xA0;
    stCIP.irDriveRegs.bCommandReg = SMART_CMD;
    bRet = DeviceIoControl(hDisk, SMART_RCV_DRIVE_DATA, &stCIP, sizeof(stCIP), szAttributes, sizeof(SENDCMDOUTPARAMS) + READ_ATTRIBUTE_BUFFER_SIZE - 1, &dwRet, NULL);
    if (bRet)
    {
        Disk->SmartValuesCount = 0;
        pT1 = (PBYTE)(((SENDCMDOUTPARAMS*)szAttributes)->bBuffer);
        for (ucT1 = 0; ucT1 < 30; ++ucT1)//проходим по всем аттрибутам
        {
            pT3 = &pT1[2 + ucT1 * 12];
            pT2 = (PDWORD)&pT3[INDEX_ATTRIB_RAW];
            pT3[INDEX_ATTRIB_RAW + 2] = pT3[INDEX_ATTRIB_RAW + 3] = pT3[INDEX_ATTRIB_RAW + 4] = pT3[INDEX_ATTRIB_RAW + 5] = pT3[INDEX_ATTRIB_RAW + 6] = 0;
            if (pT3[INDEX_ATTRIB_INDEX] != 0)
            {
                //записываем аттрибут в память
                pSmartValues = &Disk->DiskSMART[Disk->SmartValuesCount];
                pSmartValues->Index = pT3[INDEX_ATTRIB_INDEX];
                pSmartValues->Value = pT3[INDEX_ATTRIB_VALUE];
                pSmartValues->Worst = pT3[INDEX_ATTRIB_WORST];
                pSmartValues->RAWValue = pT2[0];
                pSmartValues->Threshold = MAXDWORD;
                if (pT3[INDEX_ATTRIB_INDEX] == SMART_TEMPERATURE)
                {
                    //выносим температуру в отдельную переменную для удобства
                    if (pT2[0] > 200)
                    {
                        //для некоторых жестких дисков Seagete или Hitachi
                        Disk->TempCurrent = pT3[INDEX_ATTRIB_RAW];
                    }
                    else
                    {
                        Disk->TempCurrent = (BYTE)pSmartValues->RAWValue;
                    }
                }
                Disk->SmartValuesCount++;
            }
        }
    }
    else
        dwRet = GetLastError();
    //запрашиваем пороговые значения аттрибутов
    stCIP.irDriveRegs.bFeaturesReg = READ_THRESHOLDS;
    stCIP.cBufferSize = READ_THRESHOLD_BUFFER_SIZE;
    bRet = DeviceIoControl(hDisk, SMART_RCV_DRIVE_DATA, &stCIP, sizeof(stCIP), szAttributes, sizeof(SENDCMDOUTPARAMS) + READ_ATTRIBUTE_BUFFER_SIZE - 1, &dwRet, NULL);
    if (bRet)
    {
        pT1 = (PBYTE)(((SENDCMDOUTPARAMS*)szAttributes)->bBuffer);
        int i = 0;
        for (ucT1 = 0; ucT1 < 30; ++ucT1)
        {
            pT2 = (PDWORD)&pT1[2 + ucT1 * 12 + 1];
            pT3 = &pT1[2 + ucT1 * 12];
            pT3[INDEX_ATTRIB_RAW + 2] = pT3[INDEX_ATTRIB_RAW + 3] = pT3[INDEX_ATTRIB_RAW + 4] = pT3[INDEX_ATTRIB_RAW + 5] = pT3[INDEX_ATTRIB_RAW + 6] = 0;
            if (pT3[INDEX_ATTRIB_INDEX] != 0)
            {
                //записываем в память
                pSmartValues = &Disk->DiskSMART[i];
                pSmartValues->Threshold = (BYTE)pT2[INDEX_ATTRIB_INDEX];
                i++;
            }
        }
    }
    return bRet;
}

void GetDiskHealth(DISK* Disk)
{
    BYTE CriticalNum = 0;

    if (Disk == nullptr)
        return;

    for (BYTE i = 0; i < Disk->SmartValuesCount; i++)
    {
        if (Disk->DiskSMART[i].Value < Disk->DiskSMART[i].Threshold)
        {
            CriticalNum += 20;  
        }
        if (Disk->DiskSMART[i].RAWValue > 0)
        {
            if (Disk->DiskSMART[i].Index == SMART_REALLOCATED_SECTOR_COUNT ||
                Disk->DiskSMART[i].Index == SMART_SEEK_ERROR_RATE ||
                Disk->DiskSMART[i].Index == SMART_SPIN_RETRY_COUNT ||
                Disk->DiskSMART[i].Index == SMART_CURRENT_PENDING_SECTOR_COUNT ||
                Disk->DiskSMART[i].Index == SMART_REALLOCATION_EVENT_COUNT||
                Disk->DiskSMART[i].Index == SMART_UNCORRECTABLE_SECTOR_COUNT ||
                Disk->DiskSMART[i].Index == SMART_WRITE_ERROR_RATE||
                Disk->DiskSMART[i].Index == SMART_REPORTED_UNC_ERRORS
                )
            {
                CriticalNum += 5;
            }
            if (Disk->DiskSMART[i].Index == SMART_DISK_SHIFT ||
                Disk->DiskSMART[i].Index == SMART_G_SENSE_ERROR_RATE ||
                Disk->DiskSMART[i].Index == SMART_GSENSE_ERROR_RATE ||
                Disk->DiskSMART[i].Index == SMART_FREE_FALL_EVENT_COUNT
                )
            {
                CriticalNum += 50;
            }
        }
    }
    if (CriticalNum < 10)
        Disk->HealthStatus = HEALTH_NORMAL;
    else if (CriticalNum < 30)
        Disk->HealthStatus = HEALTH_CAUTION;
    else if (CriticalNum < 50)
        Disk->HealthStatus = HEALTH_BAD;
    else
        Disk->HealthStatus = HEALTH_CRITICAL;
}

BOOL OutDisksToList(DISK* Disks, HWND hList)
{
    LVITEM lItem;
    ZeroMemory(&lItem, sizeof(LVITEM)); 
    WCHAR Buff[40];
    lItem.mask = LVIF_TEXT;
    lItem.pszText = Buff;
    lItem.cchTextMax = 40;
    if (Disks == nullptr)
        return FALSE;
    //выводим информацию для каждого диска
    for (unsigned int i = 0, j = 0; i < Disks[0].DisksCount; i++)
    {
        if (Disks[i].IsValid == TRUE)
        {
            lItem.iItem = j;
            lItem.iSubItem = 0;
            wsprintf(Buff, L"%u", Disks[i].DiskNumber);
            ListView_InsertItem(hList, &lItem);

            lItem.iSubItem = 1;
            wsprintf(Buff, L"%lu", (unsigned int)(Disks[i].Capacity / (1024 * 1024 * 1024)));
            ListView_SetItem(hList, &lItem);

            lItem.iSubItem = 2;
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&Disks[i].DiskName, 40, Buff, 40);
            ListView_SetItem(hList, &lItem);
            j++;
        }
        
    }
    return TRUE;
}

BOOL OutDiskInfoToList(DISK* Disk, HWND hList)
{
    LVITEM lItem;
    ZeroMemory(&lItem, sizeof(LVITEM));
    WCHAR Buff[40];
    lItem.mask = LVIF_TEXT;
    lItem.pszText = Buff;
    lItem.cchTextMax = 40;
    if (Disk == nullptr)
        return FALSE;
    //выводим информацию про даный конкретный диск
    lItem.iItem = 0;
    lItem.iSubItem = 0;
    wsprintf(Buff, L"Модель");
    ListView_InsertItem(hList, &lItem);
    
    lItem.iSubItem = 1;
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&Disk->DiskName, 40, Buff, 40);
    ListView_SetItem(hList, &lItem);
    
    lItem.iItem++;
    lItem.iSubItem--;
    wsprintf(Buff, L"Объем");
    ListView_InsertItem(hList, &lItem);

    lItem.iSubItem++;
    wsprintf(Buff, L"%u", (unsigned int)(Disk->Capacity / (1024 * 1024 * 1024)));
    lstrcat(Buff, L" Гб");
    ListView_SetItem(hList, &lItem);

    lItem.iItem++;
    lItem.iSubItem--;
    wsprintf(Buff, L"Серийный номер");
    ListView_InsertItem(hList, &lItem);

    lItem.iSubItem = 1;
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&Disk->SerialNumber, 32, Buff, 40);
    ListView_SetItem(hList, &lItem);

    lItem.iItem++;
    lItem.iSubItem--;
    wsprintf(Buff, L"Ревизия");
    ListView_InsertItem(hList, &lItem);

    lItem.iSubItem = 1;
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&Disk->Firmware, 16, Buff, 40);
    ListView_SetItem(hList, &lItem);

    lItem.iItem++;
    lItem.iSubItem--;
    wsprintf(Buff, L"Производитель");
    ListView_InsertItem(hList, &lItem);

    lItem.iSubItem = 1;
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&Disk->Vendor, 8, Buff, 40);
    ListView_SetItem(hList, &lItem);

    lItem.iItem++;
    lItem.iSubItem--;
    wsprintf(Buff, L"Байт в секторе");
    ListView_InsertItem(hList, &lItem);

    lItem.iSubItem = 1;
    wsprintf(Buff, L"%u", Disk->BytesPerSector);
    ListView_SetItem(hList, &lItem);
    
    return TRUE;
}

BOOL OutDiskSMARTToList(DISK* CurDisk, HWND hList, SETTINGS &cSettings)
{
    LVITEM lItem;
    ZeroMemory(&lItem, sizeof(LVITEM));
    WCHAR Buff[40];
    lItem.mask = LVIF_TEXT;
    lItem.pszText = Buff;
    lItem.cchTextMax = 40;
    if (CurDisk == nullptr)
        return FALSE;
    //выводим все аттрибуты данного диска
    for (unsigned int i = 0; i < CurDisk->SmartValuesCount; i++)
    {
        lItem.iItem = i;
        lItem.iSubItem = 0;
        wsprintf(Buff, L"%u", CurDisk->DiskSMART[i].Index);
        ListView_InsertItem(hList, &lItem);


        lItem.iSubItem = 1;
        wsprintf(Buff, L"%s", SmartToWstrParametr(CurDisk->DiskSMART[i].Index));
        ListView_SetItem(hList, &lItem);
 
        lItem.iSubItem = 2;
        wsprintf(Buff, L"%u", CurDisk->DiskSMART[i].Worst);
        ListView_SetItem(hList, &lItem);

        lItem.iSubItem = 3;
        wsprintf(Buff, L"%u", CurDisk->DiskSMART[i].Threshold);
        ListView_SetItem(hList, &lItem);

        lItem.iSubItem = 4;
        wsprintf(Buff, L"%u", CurDisk->DiskSMART[i].Value);
        ListView_SetItem(hList, &lItem);

        lItem.iSubItem = 5;
        
        if (cSettings.DisplayHEX == TRUE)//выводим RAW значение либо в десятичном либо в 16-ричном
        {
            wsprintf(Buff, L"%#.8x", CurDisk->DiskSMART[i].RAWValue);
        }
        else 
        {
            wsprintf(Buff, L"%u", CurDisk->DiskSMART[i].RAWValue);
        }
        ListView_SetItem(hList, &lItem);
    }
    return TRUE;
}

void OutDiskHealthToEdit(DISK* Disk, HWND hList)
{
    if (Disk == nullptr)
        return;
    //выводим статус диска
    switch (Disk->HealthStatus)
    {
    case HEALTH_NORMAL:
        SetWindowText(hList,L"Хорошо");
        return;
    case HEALTH_CAUTION:
        SetWindowText(hList, L"Внимание!");
        return;
    case HEALTH_BAD:
        SetWindowText(hList, L"Плохо!");
        return;
    case HEALTH_CRITICAL:
        SetWindowText(hList, L"КРИТИЧЕСКИ!");
        return;
    }
}

void AddToAutoStart(BOOL Add)
{
    HKEY Reg;
    DWORD dRet;
    LSTATUS sRet;
    WCHAR AppPath[MAX_PATH];
    WCHAR* tAppPath;
    WCHAR AppName[] = L"HDDmonitor";
    //в ветвь автозапуска реестра
    sRet = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS| KEY_WOW64_32KEY, &Reg);
    if (sRet != ERROR_SUCCESS)
    {
        MessageBox(NULL, L"Error while adding to auto start", L"Error!", MB_OK);
        return;
    }

    if (Add == TRUE)
    {
        dRet = GetModuleFileNameW(NULL, AppPath, MAX_PATH);
        if (dRet == MAX_PATH)
        {
            MessageBox(NULL, L"Error, path too long!", L"Error!", MB_OK);
            return;
        }
        tAppPath = new WCHAR[MAX_PATH + 1];
        wsprintf(tAppPath, L"\"%s\"-h", AppPath);
        //записываем имя исполняемого файла с аттрибутом фонового запуска
        sRet = RegSetValueExW(Reg, AppName, NULL, REG_SZ, (LPBYTE)tAppPath, sizeof(AppPath));
        delete[] tAppPath;
        if (sRet != ERROR_SUCCESS)
        {
            MessageBox(NULL, L"Error while adding to auto start", L"Error!", MB_OK);
            return;
        }
    }
    else if (Add == FALSE)
    {
        //удаляем из автозагрузки, если флаг в настройках убран
        dRet = RegDeleteValueW(Reg, AppName);
    }
    RegCloseKey(Reg);
}

void TimerFunc(HWND unnamedParam1, UINT unnamedParam2, UINT_PTR unnamedParam3, DWORD unnamedParam4)
{  
    time_t NowTime = time(NULL);
    //выясняем, сколько прошло с последней проверки
    double diff = difftime(NowTime, CurrentSettings.LastCheckTime);
    double desirebleDiff = 0;
    //выясняем сколько должно было пройти
    switch (CurrentSettings.CheckPeriod)
    {
    case Period30min:
        desirebleDiff = 1800;
        break;
    case Period1h:
        desirebleDiff = 3600;
        break;
    case Period5h:
        desirebleDiff = 18000;
        break;
    case Period12h:
        desirebleDiff = 43200;
        break;
    case Period1d:
        desirebleDiff = 86400;
        break;
    case Period3d:
        desirebleDiff = 259200;
        break;
    case Period7d:
        desirebleDiff = 604800;
        break;
    }
    //если достаточно - проводим проверку
    if (diff > desirebleDiff)
    {
        if (ListDisks != nullptr)
            delete[] ListDisks;
        ListDisks = GetDisks();
        CurrentSettings.LastCheckTime = time(NULL);

        for (DWORD i = 0; i < ListDisks[0].DisksCount; i++)
        {
            if (ListDisks[i].IsValid)
            {
                switch (ListDisks[i].HealthStatus)
                {
                    //вывод уведомления в трей о состоянии здоровья диска, если есть проблемы
                case HEALTH_CAUTION:
                    nIData.uFlags = NIF_INFO | NIF_GUID;
                    wsprintf(nIData.szInfo, L"С одним из ваших жестких дисков возникла проблема");
                    wsprintf(nIData.szInfoTitle, L"Внимание");
                    nIData.dwInfoFlags = NIIF_WARNING | NIIF_RESPECT_QUIET_TIME;
                    Shell_NotifyIcon(NIM_MODIFY, &nIData);
                    break;
                case HEALTH_BAD:
                    nIData.uFlags = NIF_INFO | NIF_GUID;
                    wsprintf(nIData.szInfo, L"Один из ваших жестких дисков находится в опасном состоянии!");
                    wsprintf(nIData.szInfoTitle, L"Внимание!");
                    nIData.dwInfoFlags = NIIF_ERROR;
                    Shell_NotifyIcon(NIM_MODIFY, &nIData);
                    break;
                case HEALTH_CRITICAL:
                    nIData.uFlags = NIF_INFO | NIF_GUID;
                    wsprintf(nIData.szInfo, L"Один из жестких дисков серьезно поврежден! Немедленно выключите компьютер!");
                    wsprintf(nIData.szInfoTitle, L"КРИТИЧЕСКАЯ НЕИСПРАВНОСТЬ!");
                    nIData.dwInfoFlags = NIIF_ERROR;
                    Shell_NotifyIcon(NIM_MODIFY, &nIData);
                    break;
                }
            }
        }
    }
}