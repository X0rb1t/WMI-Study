#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#include <string>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "wbemuuid.lib")

int main() {

    HRESULT hres;
    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        std::cout << "Falha ao inicializar a COM Library." << std::endl;
        return 1;
    }

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

    if (FAILED(hres)) {
        std::cout << "Falha ao inicializar a seguran�a." << std::endl;
        CoUninitialize();
        return 1;
    }

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres)) {
        std::cout << "Falha ao criar uma inst�ncia IWbemLocator." << std::endl;
        CoUninitialize();
        return 1;
    }
    while (true) {
        // Loop para gerar os nomes das m�quinas F00400, F00401, ..., F00499
        for (int i = 0; i <= 99; ++i) {
            std::stringstream machineName;
            machineName << "F004" << std::setw(2) << std::setfill('0') << i;
            std::wstring machineNameWStr = std::wstring(machineName.str().begin(), machineName.str().end());
            std::wcout << L"Processando a m�quina: " << machineNameWStr << std::endl;
            BSTR strMachineName = SysAllocStringLen(machineNameWStr.data(), machineNameWStr.size());
            IWbemServices* pSvc = NULL;
            hres = pLoc->ConnectServer(strMachineName, NULL, NULL, 0, NULL, 0, 0, &pSvc);
            SysFreeString(strMachineName);

            if (FAILED(hres)) {
                std::wcout << L"Falha ao conectar � m�quina " << machineNameWStr << std::endl;
                continue;
            }

            IEnumWbemClassObject* pEnumerator = NULL;
            hres = pSvc->ExecQuery(BSTR(L"WQL"), BSTR(L"SELECT * FROM Win32_OperatingSystem WHERE Primary = 'true'"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);

            if (FAILED(hres)) {
                std::wcout << L"Falha na consulta WMI para a m�quina " << machineNameWStr << std::endl;
                pSvc->Release();
                continue;
            }

            IWbemClassObject* pclsObj = NULL;
            ULONG uReturn = 0;

            while (pEnumerator) {
                hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (0 == uReturn) {
                    break;
                }

                VARIANT vtProp;
                IWbemClassObject* pInParams = NULL;
                IWbemClassObject* pOutParams = NULL;
                BSTR MethodName = SysAllocString(L"Win32Shutdown");
                BSTR ClassName = SysAllocString(L"Win32_OperatingSystem");

                hres = pSvc->GetObject(ClassName, 0, NULL, &pclsObj, NULL);
                if (SUCCEEDED(hres)) {
                    hres = pclsObj->GetMethod(MethodName, 0, &pInParams, NULL);
                    if (SUCCEEDED(hres)) {
                        hres = pSvc->ExecMethod(ClassName, MethodName, 0, NULL, pInParams, &pOutParams, NULL);
                        if (SUCCEEDED(hres)) {
                            std::wcout << L"Comando de desligamento enviado com sucesso para a m�quina " << machineNameWStr << std::endl;
                        }
                        else {
                            std::wcout << L"Falha ao enviar o comando de desligamento para a m�quina " << machineNameWStr << std::endl;
                        }
                    }
                }

                SysFreeString(MethodName);
                SysFreeString(ClassName);
                if (pInParams) pInParams->Release();
                if (pOutParams) pOutParams->Release();
            }
            pEnumerator->Release();
            pSvc->Release();

            if (SUCCEEDED(hres)) {
                std::wcout << L"Comando de desligamento enviado com sucesso para a m�quina " << machineNameWStr << std::endl;
            }
            else {
                std::wcout << L"Falha ao enviar o comando de desligamento para a m�quina " << machineNameWStr << std::endl;
            }
        }
    }

    pLoc->Release();
    CoUninitialize();

    return 0;
}
          