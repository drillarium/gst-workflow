//---------------------------------------------------------------------------
// MLProtect_MFormatsSDK.h : Personal protection code for the Medialooks License system
//---------------------------------------------------------------------------
// Copyright (c) 2018, Medialooks Soft
// www.medialooks.com (dev@medialooks.com)
//
// Authors: Medialooks Team
// Version: 3.0.0.0
//
//---------------------------------------------------------------------------
// CONFIDENTIAL INFORMATION
//
// This file is Intellectual Property (IP) of Medialooks Soft and is
// strictly confidential. You can gain access to this file only if you
// sign a License Agreement and a Non-Disclosure Agreement (NDA) with
// Medialooks Soft. If you had not signed any of these documents, please
// contact <dev@medialooks.com> immediately.
//
//---------------------------------------------------------------------------
// Usage:
//
// 1. Include MLProtect_MFormatsSDK.h in your C++ application, e.g.
//    #include "MLProtect_MFormatsSDK.h"
//
// 2. Call IntializeProtection() method before creating any Medialooks objects
//    For e.g. after ::CoInitialize() call:
//    ...
//	  HRESULT hRes = ::CoInitialize(NULL);
//	  MFormatsSDKLic::IntializeProtection();
//    ...
//
// 3. Call CloseProtection() method at the application shutdown
//    For e.g. before ::CoUninitialize() call:
//    ...
//	  MFormatsSDKLic::CloseProtection();
//    HRESULT hRes = ::CoUninitialize();
//    ...
//
// 4. Compile the application
//
// IMPORTANT: If you have several Medialooks products, don't forget to initialize
//            protection for all of them. For e.g.
//
//            MPlatformSDKLic.IntializeProtection();
//            DecoderlibLic.IntializeProtection();
//            etc.

#ifndef _ML_PROTECT_MFormatsSDK_H_
#define _ML_PROTECT_MFormatsSDK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "windows.h"

//////////////////////////////////////////////////////////////////////////
// MLProxy interface definition

#ifndef _MLPROXY_INTERFACE_DEF_
#define _MLPROXY_INTERFACE_DEF_

//#include "oaidl.h"
//#include "ocidl.h"

class DECLSPEC_UUID("988CDFB8-09F0-43D4-BB86-B1613F41D940")	CoMLProxy;
    
MIDL_INTERFACE("24465C14-816A-41E3-BC83-E1A04D502CF3")
IMLProxy : public IDispatch
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetData( /* [out] */ int *_pnFirst,/* [out] */ int *_pnSecond) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetData( /* [in] */ int _nFirst,/* [in] */ int _nSecond,
		/* [in] */ int _nFirstRes, /* [in] */ int _nSecondRes) = 0;
};

MIDL_INTERFACE("24465C17-816A-41E3-BC83-E1A04D502CF3")
IMLProxy2 : public IMLProxy
{
public:
	virtual HRESULT STDMETHODCALLTYPE PutString( 
		 /* [in] */ BSTR _bsString) = 0;
};

#endif // _MLPROXY_INTERFACE_DEF_

class MFormatsSDKLic
{
public:
	// Proxy object 
	static IMLProxy*	s_pMLProxy;
	static LPCOLESTR	s_pstrLicInfo;

public:
	static inline HRESULT IntializeProtection()
	{
		HRESULT hr = S_OK;
		if(s_pMLProxy == NULL)
		{
			hr = ::CoCreateInstance(__uuidof(CoMLProxy), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMLProxy), (void**)&s_pMLProxy);
			if(hr != S_OK)
				return hr;

			IMLProxy2* pMLProxy2 = NULL;
			s_pMLProxy->QueryInterface(&pMLProxy2);
			if(pMLProxy2 != NULL)
			{
				BSTR bstr = ::SysAllocString(s_pstrLicInfo);
				if(bstr != NULL)
				{
					pMLProxy2->PutString(bstr);
					SysFreeString(bstr);
				}
				pMLProxy2->Release();
			}
		}
		hr = UpdatePersonalProtection();
		return hr;
	}

	static void CloseProtection()
	{
		if(s_pMLProxy != NULL)
		{
			s_pMLProxy->Release();
			s_pMLProxy = NULL;
		}
	}

private:
	////////////////////////////////////////////////////////////////////////
	// MediaLooks License secret key
	// Issued to: NRD Multimedia, S.L.

#define ___Q1___	(34151669)
#define ___P1___	(63257791)
#define ___Q2___	(38696687)
#define ___P2___	(53885339)

	static inline int SummBits( UINT _nValue )
	{
		int nRes = 0;
		while( _nValue > 0 )
		{
			nRes += (_nValue & 1);
			_nValue >>= 1;
		}

		return nRes % 2;
	}

	static inline HRESULT UpdatePersonalProtection()
	{
		if( !s_pMLProxy )
			return E_NOINTERFACE;

		int nFirst = 0;
		int nSecond = 0;
		s_pMLProxy->GetData( &nFirst, &nSecond );

		// Calculate First * Q1 mod P1
		ULONGLONG llFirst = (ULONGLONG)nFirst * ___Q1___ % ___P1___;
		// Calculate Second * Q2 mod P2
		ULONGLONG llSecond = (ULONGLONG)nSecond * ___Q2___ % ___P2___;

		UINT uRes = SummBits( (UINT)(llFirst + llSecond) );

		// Calculate check value
		ULONGLONG llCheck = (ULONGLONG)(nFirst - 29)*(nFirst-23) % nSecond;
		// Calculate return value
		srand( nFirst );
		int nValue = (int)llCheck + (int)rand() * (uRes ? 1 : -1);

		s_pMLProxy->SetData( nFirst, nSecond, (int)llCheck, nValue );
		// (keep object in memory) pMLProxy->Release();
			
		return S_OK;
	}

#undef ___Q1___	
#undef ___P1___	
#undef ___Q2___	
#undef ___P2___	

};

_declspec(selectany) IMLProxy* MFormatsSDKLic::s_pMLProxy = NULL;

_declspec(selectany) LPCOLESTR MFormatsSDKLic::s_pstrLicInfo = L"[MediaLooks]\n"
		L"License.ProductName=MFormats SDK\n"
		L"License.IssuedTo=NRD Multimedia, S.L.\n"
		L"License.CompanyID=9782\n"
		L"License.UUID={D33254F7-41D2-4B43-A4B1-34D2A603BC17}\n"
		L"License.Key={4ABF2F07-9F60-2D49-40D4-0231EC064532}\n"
		L"License.Name=MFormats Module\n"
		L"License.UpdateExpirationDate=May 25, 2023\n"
		L"License.Edition=io_prof Professional gpu_h264 gpu_pipeline\n"
		L"License.AllowedModule=*.*\n"
		L"License.Signature=BBA7AFC8C5BFC0D141C1FC85CF99C61327573F7ED47B29879C80C4DA8E06DC651D3B54A77610FC8B243412468575D884FA06041C16869AA224A1D4C879895109E39CC407ACDB3387C1F8C77CB62B09904957B58A4BFB55B482C9971E31DDF2B7DE871FBDA2529290DEDF9B60CF77DB526E84D83CFE598D3938D1421FACAA9789\n"
		L"[MediaLooks]\n"
		L"License.ProductName=MFormats SDK\n"
		L"License.IssuedTo=NRD Multimedia, S.L.\n"
		L"License.CompanyID=9782\n"
		L"License.UUID={5BD7665B-7A84-4995-B7F2-0D2B230113FE}\n"
		L"License.Key={18C3E01E-82CF-9415-718D-4AF435A2AB72}\n"
		L"License.Name=MFRenderer Module\n"
		L"License.UpdateExpirationDate=May 25, 2023\n"
		L"License.Edition=io_prof Professional gpu_h264 gpu_pipeline\n"
		L"License.AllowedModule=*.*\n"
		L"License.Signature=9E6B65E4F5A1B22C4D724A662FC4CAD967C8CD6CFDD7338430A9FD3016475B968C349DFD6D5EC20533C53D5C4C5607C5DC275640F2AE34EA0BC4408EF56A922B1CBC217FCC89C2AA27A5B42342136093E7DDC2ED4966A3FF5FFBC991EE1F35EC5430D45FB325D776DFE99E9F0E68A4260D357914AC630DADC385B935112234CE\n"
		L"[MediaLooks]\n"
		L"License.ProductName=MFormats SDK\n"
		L"License.IssuedTo=NRD Multimedia, S.L.\n"
		L"License.CompanyID=9782\n"
		L"License.UUID={85DF9E32-9E29-4E22-B5C5-FE1E85A9AE83}\n"
		L"License.Key={FFEEA4C3-BD90-EC2E-C951-9074F8931850}\n"
		L"License.Name=MFReader Module\n"
		L"License.UpdateExpirationDate=May 25, 2023\n"
		L"License.Edition=io_prof Professional gpu_h264 gpu_pipeline\n"
		L"License.AllowedModule=*.*\n"
		L"License.Signature=495E53F015373910F77EA347AFBBCF907B7F7A55EC7B5E5E36CDC113130F80E06535C423757C086D20A17CD46027ABCE74CB47E30D7D5CB6F51C0E3E114B55AFBEB13C944C15E2FB5A7D57A97309EA4CD06797BBAB472928AB8FC3B84CB0E72B6B5A549E00A9CD4AF49504186716C033927ED93E902172B147EE967844D899C3\n"
		L"[MediaLooks]\n"
		L"License.ProductName=MFormats SDK\n"
		L"License.IssuedTo=NRD Multimedia, S.L.\n"
		L"License.CompanyID=9782\n"
		L"License.UUID={13F4236D-0194-451A-9B39-4C83FB300FB9}\n"
		L"License.Key={20895D49-7E3F-BAA3-58C9-AE67BACA0D84}\n"
		L"License.Name=MFWriter Module\n"
		L"License.UpdateExpirationDate=May 25, 2023\n"
		L"License.Edition=io_prof Professional gpu_h264 gpu_pipeline\n"
		L"License.AllowedModule=*.*\n"
		L"License.Signature=A31478B4ED765F9027F83A9A0DF013AC61796F43A4DD34E8F5975A0EA5E3628742421FEA603A40210AB33D81F4473A7415B359A6E9798E71ECA5A5A8DBD609C69554DB60E04D19CA1251F3CB91CDF4FDE762D3FBE0354A2CAB7A9C13DBC1DB81575AF80F8AEC5DA50B282AB214A96DC44E845F8ADEDDB81C5DD9D083DCCDB907\n"
		L"[MediaLooks]\n"
		L"License.ProductName=MFormats SDK\n"
		L"License.IssuedTo=NRD Multimedia, S.L.\n"
		L"License.CompanyID=9782\n"
		L"License.UUID={3B123470-2062-4EDE-8D90-6B428C697892}\n"
		L"License.Key={665BA3FC-BC44-F745-6F62-467984AF570B}\n"
		L"License.Name=Delay internal module\n"
		L"License.UpdateExpirationDate=May 25, 2023\n"
		L"License.Edition=io_prof Professional gpu_h264 gpu_pipeline\n"
		L"License.AllowedModule=*.*\n"
		L"License.Signature=F69B0BC8C2C1365EFBCD8103FDF27689D6802BD40ABA6034C0EC8068E5EC7BED7E1FD2DAED92238BE838E20192DE41B805FF465B297C08A7722F32147E513068B777051F19A237D882D52E006DECE9E428D4BFF3096F414AE43E8DA67FFDFC2EF137EDB21E778D4D6B0A91B47E3C9928F3171FB68FF701DEA145990C4D8702A1\n"
		L"[MediaLooks]\n"
		L"License.ProductName=MFormats SDK\n"
		L"License.IssuedTo=NRD Multimedia, S.L.\n"
		L"License.CompanyID=9782\n"
		L"License.UUID={D921C9D9-C731-45B5-BFC8-684EA669DF7F}\n"
		L"License.Key={5FEB4E28-4590-7C9D-2E0C-E9124728B12A}\n"
		L"License.Name=MWebRTC module\n"
		L"License.UpdateExpirationDate=May 25, 2023\n"
		L"License.Edition=Standard gpu_h264 gpu_pipeline\n"
		L"License.AllowedModule=*.*\n"
		L"License.Signature=F3FA2FAF7E1B0EDE905611A251B74F520180FA83EFB77650B8A54B756B2FF71EBBB757F5DE38C8BF1637EBE316A86B9FAD115D5E5AACFF200B58589D26270103105F75F4AD2EA5A48306A7D98E193933DC38D88918E1E1836C51A5F3D2DD75DE09B47FADBCE13F58313D25CE4C4D66F0D5C798E7EA2EA8FF1FA2C738AB74BEBF\n"
		L"[MediaLooks]\n"
		L"License.ProductName=MFormats SDK\n"
		L"License.IssuedTo=NRD Multimedia, S.L.\n"
		L"License.CompanyID=9782\n"
		L"License.UUID={08FA4C9C-F603-41E5-84DD-61E75B010548}\n"
		L"License.Key={D6214FA8-6BF6-CD53-2595-C80039829788}\n"
		L"License.Name=Medialooks DXGI Screen Capture\n"
		L"License.UpdateExpirationDate=May 25, 2023\n"
		L"License.Edition=Standard\n"
		L"License.AllowedModule=*.*\n"
		L"License.Signature=F9D0915F2E9543318656A857B5999BD3DD26DE76CF0632E2036788D073969E4D6D45DC2A22D13C03E9BFFF57BD7A77F70C232DFFBCADBC06C99F3404DA9CFD72F9D355997793ABF572A3FF421111516DA99F465D338ADD3CC798C354C634BB86560465A6E213F7661E8DD0EA3C754BC513FA5C7A23AB491A71AD3606698E857E\n";

#endif // _ML_PROTECT_H_