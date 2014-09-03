// Face.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
// This source code is licensed under the MIT license. Please see the License in License.txt.
// "This is preliminary software and/or hardware and APIs are preliminary and subject to change."
//

#include "stdafx.h"
#include <Windows.h>
#include <Kinect.h>
#include <Kinect.Face.h>
#include <opencv2/opencv.hpp>

#define _USE_MATH_DEFINES
#include <math.h>


template<class Interface>
inline void SafeRelease( Interface *& pInterfaceToRelease )
{
	if( pInterfaceToRelease != NULL ){
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

// Quote from Kinect for Windows SDK v2.0 Developer Preview - Samples/Native/FaceBasics-D2D, and Partial Modification
// ExtractFaceRotationInDegrees is: Copyright (c) Microsoft Corporation. All rights reserved.
inline void ExtractFaceRotationInDegrees( const Vector4* pQuaternion, int* pPitch, int* pYaw, int* pRoll )
{
	double x = pQuaternion->x;
	double y = pQuaternion->y;
	double z = pQuaternion->z;
	double w = pQuaternion->w;

	// convert face rotation quaternion to Euler angles in degrees
	*pPitch = static_cast<int>( std::atan2( 2 * ( y * z + w * x ), w * w - x * x - y * y + z * z ) / M_PI * 180.0f );
	*pYaw = static_cast<int>( std::asin( 2 * ( w * y - x * z ) ) / M_PI * 180.0f );
	*pRoll = static_cast<int>( std::atan2( 2 * ( x * y + w * z ), w * w + x * x - y * y - z * z ) / M_PI * 180.0f );
}

int _tmain( int argc, _TCHAR* argv[] )
{
	cv::setUseOptimized( true );

	// Sensor
	IKinectSensor* pSensor;
	HRESULT hResult = S_OK;
	hResult = GetDefaultKinectSensor( &pSensor );
	if( FAILED( hResult ) ){
		std::cerr << "Error : GetDefaultKinectSensor" << std::endl;
		return -1;
	}

	hResult = pSensor->Open();
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::Open()" << std::endl;
		return -1;
	}

	// Source
	IColorFrameSource* pColorSource;
	hResult = pSensor->get_ColorFrameSource( &pColorSource );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_ColorFrameSource()" << std::endl;
		return -1;
	}

	IBodyFrameSource* pBodySource;
	hResult = pSensor->get_BodyFrameSource( &pBodySource );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_BodyFrameSource()" << std::endl;
		return -1;
	}

	IFaceFrameSource* pFaceSource[BODY_COUNT];
	DWORD features = FaceFrameFeatures::FaceFrameFeatures_BoundingBoxInColorSpace
				   | FaceFrameFeatures::FaceFrameFeatures_PointsInColorSpace
				   | FaceFrameFeatures::FaceFrameFeatures_RotationOrientation
				   | FaceFrameFeatures::FaceFrameFeatures_Happy
				   | FaceFrameFeatures::FaceFrameFeatures_RightEyeClosed
				   | FaceFrameFeatures::FaceFrameFeatures_LeftEyeClosed
				   | FaceFrameFeatures::FaceFrameFeatures_MouthOpen
				   | FaceFrameFeatures::FaceFrameFeatures_MouthMoved
				   | FaceFrameFeatures::FaceFrameFeatures_LookingAway
				   | FaceFrameFeatures::FaceFrameFeatures_Glasses
				   | FaceFrameFeatures::FaceFrameFeatures_FaceEngagement;
	for( int count = 0; count < BODY_COUNT; count++ ){
		hResult = CreateFaceFrameSource( pSensor, 0, features, &pFaceSource[count] );
		if( FAILED( hResult ) ){
			std::cerr << "Error : CreateFaceFrameSource" << std::endl;
			return -1;
		}
	}

	// Reader
	IColorFrameReader* pColorReader;
	hResult = pColorSource->OpenReader( &pColorReader );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IColorFrameSource::OpenReader()" << std::endl;
		return -1;
	}

	IBodyFrameReader* pBodyReader;
	hResult = pBodySource->OpenReader( &pBodyReader );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IBodyFrameSource::OpenReader()" << std::endl;
		return -1;
	}

	IFaceFrameReader* pFaceReader[BODY_COUNT];
	for( int count = 0; count < BODY_COUNT; count++ ){
		hResult = pFaceSource[count]->OpenReader( &pFaceReader[count] );
		if( FAILED( hResult ) ){
			std::cerr << "Error : IFaceFrameSource::OpenReader()" << std::endl;
			return -1;
		}
	}

	// Description
	IFrameDescription* pDescription;
	hResult = pColorSource->get_FrameDescription( &pDescription );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IColorFrameSource::get_FrameDescription()" << std::endl;
		return -1;
	}

	int width = 0;
	int height = 0;
	pDescription->get_Width( &width ); // 1920
	pDescription->get_Height( &height ); // 1080
	unsigned int bufferSize = width * height * 4 * sizeof( unsigned char );

	cv::Mat bufferMat( height, width, CV_8UC4 );
	cv::Mat faceMat( height / 2, width / 2, CV_8UC4 );
	cv::namedWindow( "Face" );

	// Color Table
	cv::Vec3b color[BODY_COUNT];
	color[0] = cv::Vec3b( 255, 0, 0 );
	color[1] = cv::Vec3b( 0, 255, 0 );
	color[2] = cv::Vec3b( 0, 0, 255 );
	color[3] = cv::Vec3b( 255, 255, 0 );
	color[4] = cv::Vec3b( 255, 0, 255 );
	color[5] = cv::Vec3b( 0, 255, 255 );

	// Face Property Table
	std::string property[FaceProperty::FaceProperty_Count];
	property[0] = "Happy";
	property[1] = "Engaged";
	property[2] = "WearingGlasses";
	property[3] = "LeftEyeClosed";
	property[4] = "RightEyeClosed";
	property[5] = "MouthOpen";
	property[6] = "MouthMoved";
	property[7] = "LookingAway";

	// Coordinate Mapper
	ICoordinateMapper* pCoordinateMapper;
	hResult = pSensor->get_CoordinateMapper( &pCoordinateMapper );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_CoordinateMapper()" << std::endl;
		return -1;
	}

	while( 1 ){
		// Color Frame
		IColorFrame* pColorFrame = nullptr;
		hResult = pColorReader->AcquireLatestFrame( &pColorFrame );
		if( SUCCEEDED( hResult ) ){
			hResult = pColorFrame->CopyConvertedFrameDataToArray( bufferSize, reinterpret_cast<BYTE*>( bufferMat.data ), ColorImageFormat_Bgra );
			if( SUCCEEDED( hResult ) ){
				cv::resize( bufferMat, faceMat, cv::Size(), 0.5, 0.5 );
			}
		}
		SafeRelease( pColorFrame );

		// Body Frame
		cv::Point point[BODY_COUNT];
		IBodyFrame* pBodyFrame = nullptr;
		hResult = pBodyReader->AcquireLatestFrame( &pBodyFrame );
		if( SUCCEEDED( hResult ) ){
			IBody* pBody[BODY_COUNT] = { 0 };
			hResult = pBodyFrame->GetAndRefreshBodyData( BODY_COUNT, pBody );
			if( SUCCEEDED( hResult ) ){
				for( int count = 0; count < BODY_COUNT; count++ ){
					BOOLEAN bTracked = false;
					hResult = pBody[count]->get_IsTracked( &bTracked );
					if( SUCCEEDED( hResult ) && bTracked ){
						/*// Joint
						Joint joint[JointType::JointType_Count];
						hResult = pBody[count]->GetJoints( JointType::JointType_Count, joint );
						if( SUCCEEDED( hResult ) ){
							for( int type = 0; type < JointType::JointType_Count; type++ ){
								ColorSpacePoint colorSpacePoint = { 0 };
								pCoordinateMapper->MapCameraPointToColorSpace( joint[type].Position, &colorSpacePoint );
								int x = static_cast<int>( colorSpacePoint.X );
								int y = static_cast<int>( colorSpacePoint.Y );
								if( ( x >= 0 ) && ( x < width ) && ( y >= 0 ) && ( y < height ) ){
									cv::circle( bufferMat, cv::Point( x, y ), 5, static_cast< cv::Scalar >( color[count] ), -1, CV_AA );
								}
							}
						}*/

						// Set TrackingID to Detect Face
						UINT64 trackingId = _UI64_MAX;
						hResult = pBody[count]->get_TrackingId( &trackingId );
						if( SUCCEEDED( hResult ) ){
							pFaceSource[count]->put_TrackingId( trackingId );
						}
					}
				}
			}
		}
		SafeRelease( pBodyFrame );

		// Face Frame
		std::system( "cls" );
		for( int count = 0; count < BODY_COUNT; count++ ){
			IFaceFrame* pFaceFrame = nullptr;
			hResult = pFaceReader[count]->AcquireLatestFrame( &pFaceFrame );
			if( SUCCEEDED( hResult ) && pFaceFrame != nullptr ){
				BOOLEAN bFaceTracked = false;
				hResult = pFaceFrame->get_IsTrackingIdValid( &bFaceTracked );
				if( SUCCEEDED( hResult ) && bFaceTracked ){
					IFaceFrameResult* pFaceResult = nullptr;
					hResult = pFaceFrame->get_FaceFrameResult( &pFaceResult );
					if( SUCCEEDED( hResult ) && pFaceResult != nullptr ){
						UINT64 trackingId;
						hResult = pFaceResult->get_TrackingId( &trackingId );
						if( SUCCEEDED(hResult) ){
							std::cout << "Count : " << count << ", TrackingID : " << trackingId << std::endl;
						}

						// Face Bounding Box
						RectI boundingBox;
						hResult = pFaceResult->get_FaceBoundingBoxInColorSpace( &boundingBox );
						if( SUCCEEDED( hResult ) ){
							cv::rectangle( bufferMat, cv::Rect( boundingBox.Left, boundingBox.Top, boundingBox.Right - boundingBox.Left, boundingBox.Bottom - boundingBox.Top ), static_cast< cv::Scalar >( color[count] ) );
						}

						// Face Point
						PointF facePoint[FacePointType::FacePointType_Count];
						hResult = pFaceResult->GetFacePointsInColorSpace( FacePointType::FacePointType_Count, facePoint );
						if( SUCCEEDED( hResult ) ){
							cv::circle( bufferMat, cv::Point( static_cast<int>( facePoint[0].X ), static_cast<int>( facePoint[0].Y ) ), 5, static_cast< cv::Scalar >( color[count] ), -1, CV_AA ); // Eye (Left)
							cv::circle( bufferMat, cv::Point( static_cast<int>( facePoint[1].X ), static_cast<int>( facePoint[1].Y ) ), 5, static_cast< cv::Scalar >( color[count] ), -1, CV_AA ); // Eye (Right)
							cv::circle( bufferMat, cv::Point( static_cast<int>( facePoint[2].X ), static_cast<int>( facePoint[2].Y ) ), 5, static_cast< cv::Scalar >( color[count] ), -1, CV_AA ); // Nose
							cv::circle( bufferMat, cv::Point( static_cast<int>( facePoint[3].X ), static_cast<int>( facePoint[3].Y ) ), 5, static_cast< cv::Scalar >( color[count] ), -1, CV_AA ); // Mouth (Left)
							cv::circle( bufferMat, cv::Point( static_cast<int>( facePoint[4].X ), static_cast<int>( facePoint[4].Y ) ), 5, static_cast< cv::Scalar >( color[count] ), -1, CV_AA ); // Mouth (Right)
						}

						// Face Property
						DetectionResult faceProperty[FaceProperty::FaceProperty_Count];
						hResult = pFaceResult->GetFaceProperties( FaceProperty::FaceProperty_Count, faceProperty );
						if( SUCCEEDED( hResult ) ){
							for( int count = 0; count < FaceProperty::FaceProperty_Count; count++ ){
								switch( faceProperty[count] ){
									case DetectionResult::DetectionResult_Unknown:
										std::cout << property[count] << " : Unknown" << std::endl;
										break;
									case DetectionResult::DetectionResult_Yes:
										std::cout << property[count] << " : Yes" << std::endl;
										break;
									case DetectionResult::DetectionResult_No:
										std::cout << property[count] << " : No" << std::endl;
										break;
									case DetectionResult::DetectionResult_Maybe:
										std::cout << property[count] << " : Mayby" << std::endl;
										break;
									default:
										break;
								}
							}
						}

						// Face Rotation
						Vector4 faceRotation;
						hResult = pFaceResult->get_FaceRotationQuaternion( &faceRotation );
						if( SUCCEEDED( hResult ) ){
							int pitch, yaw, roll;
							ExtractFaceRotationInDegrees( &faceRotation, &pitch, &yaw, &roll );
							std::cout << "Pitch, Yaw, Roll : " << pitch << ", " << yaw << ", " << roll << std::endl;
						}

						std::cout << std::endl;
					}
					SafeRelease( pFaceResult );
				}
			}
			SafeRelease( pFaceFrame );
		}

		cv::resize( bufferMat, faceMat, cv::Size(), 0.5, 0.5 );
		cv::imshow( "Face", faceMat );

		if( cv::waitKey( 10 ) == VK_ESCAPE ){
			break;
		}
	}

	SafeRelease( pColorSource );
	SafeRelease( pBodySource );
	for( int count = 0; count < BODY_COUNT; count++ ){
		SafeRelease( pFaceSource[count] );
	}
	SafeRelease( pColorReader );
	SafeRelease( pBodyReader );
	for( int count = 0; count < BODY_COUNT; count++ ){
		SafeRelease( pFaceReader[count] );
	}
	SafeRelease( pDescription );
	SafeRelease( pCoordinateMapper );
	if( pSensor ){
		pSensor->Close();
	}
	SafeRelease( pSensor );
	cv::destroyAllWindows();

	return 0;
}
