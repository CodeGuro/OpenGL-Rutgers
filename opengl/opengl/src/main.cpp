// Moves a square using mouse and idle callbacks
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <math.h>

/*The bulk of the program*/
class Program
{
private:
	struct Vec3
	{
		float x;
		float y;
		float z;
		Vec3() {}
		Vec3( float x, float y, float z ) : x( x ), y( y ), z( z ) {}
	};
	struct Vec4
	{
		Vec4(){}
		Vec4( float x, float y, float z, float w ) : 
			x( x ), y( y ), z( z ), w( w ) {}
			Vec4( Vec3 v, float w ) : x( v.x ), y( v.y ), z( v.z ), w( w ) {}
		float x;
		float y;
		float z;
		float w;
	};
	struct Vertex
	{
		Vec3 position;
		Vec3 color;
		Vertex(){}
		Vertex( Vec3 const & pos ) : position( pos ), color( 0, 0, 0 ) {}
		Vertex( Vec3 const & pos, Vec3 const & col ) : position( pos ), color( col ) {}
	};
	struct Board /*screen width/height*/
	{
		int m_width;
		int m_height;
		Vec3 LowerBounds;
		Vec3 UpperBounds;
		Vec3 UpperBounds_Floor;
		Board() : m_width( 700 ), m_height( 700 ),
			LowerBounds( -20.f, -20.f, -20.f ), UpperBounds( 20.f, 20.f, 20.f ),
			UpperBounds_Floor( 20.f, -19.f, 20.f )
		{
		}
	};
	struct Camera
	{
	public:
		enum Trajectory
		{
			PREDEFINED_TRAJECTORY, CUSTOM_TRAJECTORY, BIRDS_EYE_VIEW
		};
		Vec3 eye;
		Vec3 at;
		Vec3 up;
		bool MotionMode;
		Trajectory TrajectoryMode;
		float Timer;
		Vec3 Storage;

		void Update()
		{
			if( MotionMode )
			{
				float PI = 2.f * acos( 0.f );
				switch( TrajectoryMode )
				{
				case PREDEFINED_TRAJECTORY:
					{
						float Quantity = 14.f * cos( Timer );
						Timer += 2 * PI / ( 10.f * 60.f );
						Timer = fmod( Timer, 2 * PI );
						eye.x = Storage.x * 16.f + Storage.x * Quantity;
						eye.y = Storage.y * 16.f + Storage.y * Quantity;
						eye.z = Storage.z * 16.f + Storage.z * Quantity;
					}
					break;
				case CUSTOM_TRAJECTORY:
					{
						Vec3 dis = Vec3( Storage.x - eye.x, Storage.y - eye.y, Storage.z - eye.z );
						float mag = sqrt( dis.x * dis.x + dis.y * dis.y + dis.z * dis.z );
						mag = mag > 1.f ? 1.f / ( 5.f * 60.f ) : 0.f;
						eye = Vec3( eye.x + dis.x * mag,
							eye.y + dis.y * mag, eye.z + dis.z * mag);
					}
					break;
				case BIRDS_EYE_VIEW:
					{
						float PI = 2 * acos( 0.f );
						Timer += 2 * PI / ( 10.f * 60.f );
						Timer = fmod( Timer, 2 * PI );
						eye.y = 12.f;
						eye.x = 12.f * sin( Timer );
						eye.z = 12.f * cos( Timer );
					}
					break;
				}
			}
		}
		Camera() : MotionMode( false )
		{
		}
	};

	class Object //abstract base class
	{
	private:
		Vec4 Orientation;
		Vec3 Position;
		Vec3 Position_Target;
		float Speed;

		Vec3 Random_Point( Vec3 const LowerBounds, Vec3 const UpperBounds )
		{
			//Find a random point within the terrarium. we will go toward this direction
			Vec3 point;
			point.x = LowerBounds.x + (float)(rand() % (int)(UpperBounds.x - LowerBounds.x) + 1);
			point.y = LowerBounds.y + (float)(rand() % (int)(UpperBounds.y - LowerBounds.y) + 1);
			point.z = LowerBounds.z + (float)(rand() % (int)(UpperBounds.z - LowerBounds.z) + 1);
			return point;
		}
		void Update_Direction()
		{
			//face the random point using quaternions and spherical linear interpolation (SLERPing)
			float PI = 2.f * acos( 0.f );
			Vec3 Direction_Target = Vec3( Position_Target.x - Position.x,
				Position_Target.y - Position.y, Position_Target.z - Position.z );
			Direction_Target = Normalize( Direction_Target );

			
			//our default orientation will be 'forward' (i.e. <0,0,1>)
			//transform this orientation by multiplying quaternions qorient * qforw * qorient'
			//to find our 'actual' forward direction currently (we may be facing 'up' or 'right', i.e. <0,1,0> or <1,0,0>)
			Vec4 dir_v4 = QuaternionMultiply( QuaternionMultiply( Orientation, Vec4( 0.f, 0.f, 1.f, 0.f ) ), QuaternionConjugate( Orientation ) );
			Vec3 Direction_Vector = Normalize( Vec3( dir_v4.x, dir_v4.y, dir_v4.z ) );

			//interpolate by slerping our actual current forward direction to the direction we need to face (our 'random point' target direction)
			Vec4 offset = QuaternionSlerp( Vec4( Direction_Vector, 0.f ), Vec4( Direction_Target, 0.f ), 1.f / ( 1.5f * 60.f ) );

			Vec4 dir = QuaternionMultiply( QuaternionMultiply( offset, Vec4( Direction_Vector, 0.f ) ),
				QuaternionConjugate( offset ) ); //multiply our actual direction by the small offset for the next frame (we are 'turning slowly' now)
			Direction_Vector = Vec3( dir.x, dir.y, dir.z );
			Vec3 Forward_Vector = Vec3( 0.f, 0.f, 1.f );


			//here we use the dot product between our 'default' forward vector and the new forward direction vector after interpolation
			//and use the cross product to rotate our orientation to the new forward vector from the 'default' forward vector <0,0,1>
			Direction_Vector = Normalize( Direction_Vector );
			Vec3 Rotation_Vector = Normalize( CrossProduct( Forward_Vector, Direction_Vector ) );
			float Rotation_Theta = 180.f * acos( DotProduct( Forward_Vector, Direction_Vector ) ) / PI;
			Orientation = QuaternionAxisAngle( Rotation_Vector, Rotation_Theta * PI / 180.f ); //our new orientation is set smoothly
		}

	protected:
		void SetSpeed( float Speed )
		{
			this->Speed = Speed;
		}
		Vec3 GetPosition() const
		{
			return Position;
		}
		Vec4 GetOrientation() const
		{
			return Orientation;
		}

	public:
		virtual void DrawFunc() = 0;
		void Draw()
		{
			//this is the fun part. All that work pays off here.
			glPushMatrix();
			glTranslatef( GetPosition().x, GetPosition().y, GetPosition().z );
			glMultMatrixf( QuaternionToMatrix( GetOrientation() ) );
			DrawFunc();
			glPopMatrix();
		}
		Object() : Orientation( 0.f, 0.f, 0.f, 1.f ), Position( 0.f, 0.f, -6.f ), Position_Target( 0.f, 0.f, 0.f ), Speed( 0.f )
		{
		}
		Object( Vec3 const LowerBounds, Vec3 const UpperBounds ) : Orientation( 0.f, 0.f, 0.f, 1.f ), Speed( 2.f )
		{
			Position = Random_Point( LowerBounds, UpperBounds );
			Position_Target = Random_Point( LowerBounds, UpperBounds );
			Update_Direction();
		}
		void Update( Vec3 const LowerBounds, Vec3 const UpperBounds )
		{
			//find the distance between our current position and the target point
			float dst = sqrt( pow( Position_Target.x - Position.x, 2.f ) + 
				pow( Position_Target.y - Position.y, 2.f ) + pow( Position_Target.z - Position.z, 2.f ) );
			if( dst <= 1.f ) //find a new point (in the terrarium) if we're close enough to it
				Position_Target = Random_Point( LowerBounds, UpperBounds );
			Update_Direction();

			//take advantage of quaternions to displace our current position in the 'forward' direction of which we're facing
			Vec4 Displace = QuaternionMultiply( QuaternionMultiply( Orientation, Vec4( 0.f, 0.f, 1.f, 0.f ) ), QuaternionConjugate( Orientation ) );
			Displace = Scale( Displace, Speed / 60.f );
			Position.x += Displace.x, Position.y += Displace.y, Position.z += Displace.z;
			
		}
	};

	enum
	{
		//for display lists
		FISH_BODY = 1,
		FISH_TAIL,
		WATERBUG_BODY,
		WATERBUG_LIMB,
		PARTICLE,
		SPHERES
	};

	class Fish : public Object
	{
	private:
		float Timer;
		float Tail_Theta;

		void DrawFunc()
		{
			glCallList( FISH_BODY );
			//following operations displace our tail. We cannot call this in a single callList() due to variable rotations
			glTranslatef( 0.f , 0.f, -1.f );
			glScalef( 1.5f, 1.5f, 1.5f );
			glRotatef( Tail_Theta, 0.f, 1.f, 0.f );
			glCallList( FISH_TAIL );
			
			float PI = 2 * acos( 0.f );
			Timer+= 2 * PI / 60.f;
			Tail_Theta = 36.f * sin( Timer );
			Timer = fmod( Timer, 2 * PI );
		}
	public:
		Fish() : Timer( (float)rand() ), Tail_Theta( 0.f )
		{
		}
		Fish( Vec3 const LowerBounds, Vec3 const UpperBounds ) : Object( LowerBounds, UpperBounds ),
			Timer( 0.f ) , Tail_Theta( 0.f )
		{
		}
	};
	class WaterBug : public Object
	{
	private:
		float Timer;
		unsigned SleepTimer;
		unsigned NextSleep;
		void DrawLeg( float Offset, float Angle )
		{
			//again, cannot call entire waterbug within a single CallList() due to unknown runtime rotations and translations
			//but now it looks very lifelike
			float PI = 2.f * acos( 0.f );
			float r1 = 30.f, r2 = 90.f;
			float dst1 = 2.f * cos( r1 * PI / 180.f  );
			float dst2 = 2.f * sin( r1 * PI / 180.f  );
			glPushMatrix();
			glTranslatef( 0.f, 0.f, Offset );
			glRotatef( Angle, 0.f, 1.f, 0.f );
			glRotatef( r1, 0.f, 0.f, 1.f );
			glRotatef( r2, 0.f, 1.f, 0.f );
			glCallList( WATERBUG_LIMB );
			glRotatef( -r2, 0.f, 1.f, 0.f );
			glRotatef( -r1, 0.f, 0.f, 1.f );
			
			glTranslatef( dst1, dst2, 0.f );
			glRotatef( -r1, 0.f, 0.f, 1.f );
			glRotatef( r2, 0.f, 1.f, 0.f );
			
			glCallList( WATERBUG_LIMB );
			glPopMatrix();
		}
		void DrawLegs()
		{
			float offsetter = 0.f;
			glPushMatrix();
			for( float f = 0.f; f < 3.f; ++f )
				for( float f2 = 0.f; f2 < 2.f; ++f2 )
					DrawLeg( f, 10.f * sin( Timer + offsetter++ ) + 180.f * f2 );
			glPopMatrix();

		}
		void DrawFunc()
		{
			//make this bug look like a real bug! have his stop randomly
			if( SleepTimer )
			{
				if( !--SleepTimer ) NextSleep = rand() % 60;
				SetSpeed( 0.f );
			}
			else
			{
				if( !NextSleep-- )
					SleepTimer = 1 + rand() % 60;
				SetSpeed( 2.f );
				float PI = 2 * acos( 0.f );
				Timer+= 4 * PI / 60.f;
				Timer = fmod( Timer, 2 * PI );
			}

			glPushMatrix();
			float scalefac = 1.f / 3.f;
			glScalef( scalefac, scalefac, scalefac );
			glCallList( WATERBUG_BODY );
			DrawLegs();	//again, cannot call in a single display list due to variables
			glPopMatrix();

		}
	public:
		WaterBug() : Timer( 0.f ), SleepTimer( 0 )
		{
		}
		WaterBug( Vec3 const LowerBounds, Vec3 const UpperBounds ) : Object( LowerBounds, UpperBounds ),
			Timer( 0.f ), SleepTimer( 0 ), NextSleep( rand() % 60 )
		{
		}
	};
	class Particle : public Object //random dirt floating in the terrarium (the cubes are supposed to be particles)
	{
	public:
		Particle( Vec3 const LowerBounds, Vec3 const UpperBounds ) : Object( LowerBounds, UpperBounds )
		{
		}
		void DrawFunc()
		{
			glCallList( PARTICLE );
		}
	};

	int WindowId;
	Board m_board;
	Camera m_camera;
	std::vector< Fish > m_fish;
	std::vector< WaterBug > m_waterbugs;
	std::vector< Particle > m_particles;

	static void DisplayFunc();
	static void ReshapeFunc( int Width, int Height );
	static void MouseFunc( int Button, int State, int posx, int posy );
	static void KeyboardFunc( unsigned char Key, int, int );
	static void SpecialFunc( int Key, int, int );
	static void TimerFunc( int Val );
	static void IdleFunc();

	//the following are math functions used for this program
	//I'm really glad my knowledge of game development in DirectX serves me here too
	//big thanks for Alexander Hamilton for inventing quaternions
	static Vec4 QuaternionMultiply( Vec4 q1, Vec4 q2 )
	{
		return Vec4( q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x,
			-q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y,
			q1.x * q2.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z,
			-q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w );
	}
	static float * QuaternionToMatrix( Vec4 quat )
	{
		//get a rotation matrix from our quaternion
		static float m[ 16 ];
		m[ 0*4 + 0 ] = m[ 1*4 + 1 ] = m[ 2*4 + 2 ] = m[ 3*4 + 3 ] = 1.f;
		m[ 0*4 + 0 ] = 1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z);
		m[ 0*4 + 1 ] = 2.0f * (quat.x *quat.y + quat.z * quat.w);
		m[ 0*4 + 2 ] = 2.0f * (quat.x * quat.z - quat.y * quat.w);
		m[ 1*4 + 0 ] = 2.0f * (quat.x * quat.y - quat.z * quat.w);
		m[ 1*4 + 1 ] = 1.0f - 2.0f * (quat.x * quat.x + quat.z * quat.z);
		m[ 1*4 + 2 ] = 2.0f * (quat.y *quat.z + quat.x *quat.w);
		m[ 2*4 + 0 ] = 2.0f * (quat.x * quat.z + quat.y * quat.w);
		m[ 2*4 + 1 ] = 2.0f * (quat.y *quat.z - quat.x *quat.w);
		m[ 2*4 + 2 ] = 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y);
		return m;
	}
	static Vec4 QuaternionLerp( Vec4 quat1, Vec4 quat2, float t )
	{
		//Linear intERPolation
		FLOAT dot, epsilon;
		Vec4 out;
		epsilon = 1.0f;
		dot = quat1.x * quat2.x + quat1.y * quat2.y + quat1.z * quat2.z + quat1.w * quat2.w;
		if ( dot < 0.0f ) epsilon = -1.0f;
		out.x = ( 1.0f - t ) * quat1.x + epsilon * t * quat2.x;
		out.y = ( 1.0f - t ) * quat1.y + epsilon * t * quat2.y;
		out.z = ( 1.0f - t ) * quat1.z + epsilon * t * quat2.z;
		out.w = ( 1.0f - t ) * quat1.w + epsilon * t * quat2.w;
		return out;
	}
	static Vec4 QuaternionSlerp( Vec4 q0, Vec4 q1, float t )
	{
		//Spherical Linear intERPolation
		float dot = q0.x*q1.x + q0.y*q1.y + q0.z*q1.z + q0.w*q1.w;

		if( dot < -1.f ) dot = -1.f;
		else if( dot > 1.f ) dot = 1.f;

		float omega = acos( dot );

		if( fabs( omega ) < 1e-10f )
		  omega = 1e-10f;

		float som = sin(omega);
		float st0 = sin((1-t) * omega) / som;
		float st1 = sin(t * omega) / som;
    
		return Vec4( q0.x*st0 + q1.x*st1,
			q0.y*st0 + q1.y*st1,
			q0.z*st0 + q1.z*st1,
			q0.w*st0 + q1.w*st1);
  }
	static Vec4 QuaternionConjugate( Vec4 qu )
	{
		return Vec4( -qu.x, -qu.y, -qu.z, qu.w );
	}
	static Vec3 TransformCoord( float const * pm, Vec3 vec ) //transform a single point
	{
		Vec3 out;
		float norm;

		//norm is our 'w' component in the vector <x,y,z,w>, it's not really for normalizing
		norm = pm[ 0*4 + 3 ] * vec.x + pm[ 1*4 + 3 ] * vec.y + pm[ 2*4 + 3 ] * vec.z + pm[ 3*4 + 3 ];

		if ( norm )
		{
			//divide by 'w' component for homogenous space
			out.x = ( pm[ 0*4 + 0 ] * vec.x + pm[ 1*4 + 0 ] * vec.y + pm[ 2*4 + 0 ] * vec.z + pm[ 3*4 + 0 ] ) / norm;
			out.y = ( pm[ 0*4 + 1 ] * vec.x + pm[ 1*4 + 1 ] * vec.y + pm[ 2*4 + 1 ] * vec.z + pm[ 3*4 + 1 ] ) / norm;
			out.z = ( pm[ 0*4 + 2 ] * vec.x + pm[ 1*4 + 2 ] * vec.y + pm[ 2*4 + 2 ] * vec.z + pm[ 3*4 + 2 ] ) / norm;
		}
		else
		{
			out.x = 0.0f;
			out.y = 0.0f;
			out.z = 0.0f;
		}
		return out;
	}
	static Vec3 CrossProduct( Vec3 vec1, Vec3 vec2 )
	{
		Vec3 vec_cross;
		vec_cross.x = vec1.y * vec2.z - vec2.y * vec1.z;
		vec_cross.y = vec1.z * vec2.x - vec2.z * vec1.x;
		vec_cross.z = vec1.x * vec2.y - vec2.x * vec1.y;
		return vec_cross;
	}
	static Vec3 Normalize( Vec3 vec )
	{
		float magnitude = sqrt( vec.x * vec.x + vec.y * vec.y + vec.z * vec.z );
		vec.x /= magnitude;
		vec.y /= magnitude;
		vec.z /= magnitude;
		return vec;
	}
	static Vec4 Normalize( Vec4 vec )
	{
		float magnitude = sqrt( vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w );
		vec.x /= magnitude;
		vec.y /= magnitude;
		vec.z /= magnitude;
		vec.w /= magnitude;
		return vec;
	}
	static Vec4 Scale( Vec4 vec, float scalefac )
	{
		if( scalefac == 0.f )
			return Vec4( 0.f, 0.f, 0.f, 0.f );
		float mag = sqrt( vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w );
		vec.x *= scalefac / mag;
		vec.y *= scalefac / mag;
		vec.z *= scalefac / mag;
		vec.w *= scalefac / mag;
		return vec;
	}
	static float DotProduct( Vec3 vec1, Vec3 vec2 )
	{
		//probably the easiest
		vec1 = Normalize( vec1 );
		vec2 = Normalize( vec2 );
		return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
	}
	static Vec4 QuaternionAxisAngle( Vec3 Axis, float Theta )
	{
		return Vec4( Axis.x * sin( Theta / 2.f ),
			Axis.y * sin( Theta / 2.f ),
			Axis.z * sin( Theta / 2.f ),
			cos( Theta / 2.f ) );
	}
	static void trajectories( int menuitem );
	static void left_menu( int menuitem );
	static void right_menu( int menuitem );

	void SetPreTrajectory( Vec3 At, Vec3 Eye )
	{
		m_camera.Timer = 0;
		m_camera.TrajectoryMode = Camera::PREDEFINED_TRAJECTORY;
		m_camera.MotionMode = true;
		m_camera.eye = Eye;
		m_camera.at = At;
		m_camera.Storage = Normalize( Vec3( Eye.x - At.x,
			Eye.y - At.y, Eye.z - At.z ) );
	}
	void SetBirdsEyeViewTrajectory()
	{
		m_camera.MotionMode = true;
		m_camera.TrajectoryMode = Camera::BIRDS_EYE_VIEW;
		m_camera.Timer = 0;
	}
	void SetCustomTrajectory()
	{
		const char * format = "%f%f%f";
		Vec3 data;

		puts( "Enter the Camera position in x y z format" );
		scanf( format, &data.x, &data.y, &data.z );
		m_camera.eye = data;

		puts( "Enter the Camera look-at location in x y z format" );
		scanf( format, &data.x, &data.y, &data.z );
		m_camera.at = data;


		puts( "Enter the Camera go-to location in x y z format" );
		scanf( format, &data.x, &data.y, &data.z );
		m_camera.Storage = data;


		m_camera.MotionMode = true;
		m_camera.TrajectoryMode = Camera::CUSTOM_TRAJECTORY;
	}

	void InitializeLists()
	{
		glMatrixMode( GL_MODELVIEW );
		
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE /*GL_FILL*/ );

		/*Spheres*/
		glPushAttrib( GL_ALL_ATTRIB_BITS );
		glNewList( SPHERES, GL_COMPILE );
		glColor3f( 1.0f, 0.0f, 0.0f );
		glPushMatrix();
		glTranslatef( -1.5f, 0.f, 0.f );
		glutSolidSphere( 1, 20, 20 );
		glPopMatrix();
		glPushMatrix();
		glTranslatef( 1.5f, 0.f, 0.f );
		glutWireSphere( 1, 20, 20 );
		glPopMatrix();
		glPushMatrix();
		glutWireSphere( 1.f, 20, 20 );
		glTranslatef( 0.f, 0.f, 2.f );
		glutWireSphere( 1.f, 20, 20 );
		glPopMatrix();
		glEndList();
		glPopAttrib();
		
		/*Fish*/
		glPushAttrib( GL_ALL_ATTRIB_BITS );
		glNewList( FISH_BODY, GL_COMPILE );
		glColor3f( 1.f, 0.f, 0.f );
		glutWireSphere( 1.f, 20, 20 );
		glEndList();

		glNewList( FISH_TAIL, GL_COMPILE );
		glColor3f( 1.f, 0.f, 0.f );
		glBegin( GL_TRIANGLES );
		glVertex3f( 0.f, 0.f, 0.f );
		glVertex3f( -0.15f, -0.5f, -0.5f );
		glVertex3f( 0.15f, -0.5f, -0.5f );
		glVertex3f( 0.f, 0.f, 0.f );
		glVertex3f( -0.15f, 0.5f, -0.5f );
		glVertex3f( 0.15f, 0.5f, -0.5f );
		glVertex3f( 0.f, 0.f, 0.f );
		glVertex3f( -0.15f, 0.5f, -0.5f );
		glVertex3f( -0.15f, -0.5f, -0.5f );
		glVertex3f( 0.f, 0.f, 0.f );
		glVertex3f( 0.15f, 0.5f, -0.5f );
		glVertex3f( 0.15f, -0.5f, -0.5f );

		glVertex3f( -0.15f, 0.5f, -0.5f );
		glVertex3f( -0.15f, -0.5f, -0.5f );
		glVertex3f( 0.15f, -0.5f, -0.5f );

		glVertex3f( -0.15f, 0.5f, -0.5f );
		glVertex3f( 0.15f, 0.5f, -0.5f );
		glVertex3f( 0.15f, -0.5f, -0.5f );
		glEnd();
		glEndList();
		glPopAttrib();

		//WaterBug
		glPushAttrib( GL_ALL_ATTRIB_BITS );
		glNewList( WATERBUG_BODY, GL_COMPILE );
		glPushMatrix();
		glColor3f( 1.f, 0.f, 0.f );
		glutWireCylinder( 0.5f, 2.f, 10, 3 );
		glTranslatef( 0.f, 0.f, 2.f );
		glScalef( 4.f / 12.f, 4.f / 12.f, 4.f / 12.f );
		glutWireDodecahedron();
		glPopMatrix();
		glEndList();

		glNewList( WATERBUG_LIMB, GL_COMPILE );
		glColor3f( 1.f, 0.f, 0.f );
		glutWireCylinder( 0.2f, 2.f, 10, 3 );
		glEndList();
		glPopAttrib();

		glNewList( PARTICLE, GL_COMPILE );
		//glutWireSphere( 0.1f, 20, 20 );
		glutWireCube( 0.1 );
		glEndList();

	}
	void DrawSpheres()
	{
		static float f = 0;
		glPushMatrix();
		glTranslatef( 0.f, 0.f, -4.f );
	//	glRotatef( f+=0.1f, 0.f, 1.f, 0.f );
		glCallList( SPHERES );
		glPopMatrix();
	}
	void Advance() /*mostly drawing*/
	{
		
		/*Set-Up*/
		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		gluLookAt( m_camera.eye.x, m_camera.eye.y, m_camera.eye.z,
			m_camera.at.x, m_camera.at.y, m_camera.at.z,
			m_camera.up.x, m_camera.up.y, m_camera.up.z );
		glPushAttrib( GL_ALL_ATTRIB_BITS );
		
		/*Drawing is performed here*/
		//DrawSpheres();
		for( unsigned u = 0; u < m_fish.size(); ++u )
			m_fish[ u ].Update( m_board.LowerBounds, m_board.UpperBounds ), m_fish[ u ].Draw();

		for( unsigned u = 0; u < m_waterbugs.size(); ++u )
			m_waterbugs[ u ].Update( m_board.LowerBounds, m_board.UpperBounds_Floor ), m_waterbugs[ u ].Draw();

		for( unsigned u = 0; u < m_particles.size(); ++u )
			m_particles[ u ].Update( m_board.LowerBounds, m_board.UpperBounds ), m_particles[ u ].Draw();

		/*Finishing*/
		glPopAttrib();
		glFlush();
		glutSwapBuffers();

		m_camera.Update();
	}
	void ReshapeWindow( int Width, int Height )
	{
		float aspect_ratio = (float)Width / (float)Height;
		static float znear = 1.f;
		static float zfar = 250.f;
		m_board.m_width = Width;
		m_board.m_height = Height;
		glViewport( 0, 0, Width, Height );
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		if( Width > Height )
			glFrustum( -aspect_ratio, aspect_ratio, -1.f, 1.f, znear, zfar );
		else
			glFrustum( -1.f, 1.f, -1.f / aspect_ratio, 1.f / aspect_ratio, znear, zfar );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
	}
	void MouseEvent( int Button, int State, int Xpos, int Ypos )
	{
		Vec3 click( (float)Xpos, (float)Ypos, 0.f );
		if( Button == GLUT_LEFT_BUTTON && State == GLUT_DOWN ) 
		{
			/*left button*/
		}
		else if( Button == GLUT_MIDDLE_BUTTON && State == GLUT_DOWN )
		{
			/*middle button*/
			return;
		}
		else if( Button == GLUT_RIGHT_BUTTON && State == GLUT_DOWN )
		{
			/*right button*/
		}
		glutPostRedisplay();
	}
	void KeyboardEvent( unsigned char Key )
	{
		float displace = 2.3f;
		switch( Key )
		{
		/*Escape / Quit*/
		case 'X':
			m_camera.eye.x += displace;
			break;
		case 'x':
			m_camera.eye.x -= displace;
			break;
		case 'Y':
			m_camera.eye.y += displace;
			break;
		case 'y':
			m_camera.eye.y -= displace;
			break;
		case 'Z':
			m_camera.eye.z += displace;
			break;
		case 'z':
			m_camera.eye.z -= displace;
			break;
		case 27:
		case 'q':
			glutDestroyWindow( WindowId );
		}
		m_camera.MotionMode = false;
		glutPostRedisplay();
	}
	void SpecialKey( int Key ) /*ignore this, it's for portability*/
	{
		switch( Key )
		{
		case GLUT_KEY_UP:
			break;
		case GLUT_KEY_LEFT:
			break;
		case GLUT_KEY_DOWN:
			break;
		case GLUT_KEY_RIGHT:
			break;
		}
		glutPostRedisplay();
	}

public:
	void RunProgram( int argc, char **argv )
	{
		/*Initialize glut*/
		glutInit( &argc, argv );
		glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );
		glutInitWindowSize( m_board.m_width, m_board.m_height );
		glutInitWindowPosition( 100, 100 );
		WindowId = glutCreateWindow( "Assignment 3" );
		glutDisplayFunc( &DisplayFunc );
		glutMouseFunc( &MouseFunc );
		glutKeyboardFunc( &KeyboardFunc );
		glutSpecialFunc( &SpecialFunc );
		glutReshapeFunc( &ReshapeFunc );
		glutTimerFunc( 16, &TimerFunc, 0 );
		glutIdleFunc( &IdleFunc );

		glEnable( GL_DEPTH_TEST );

		/*Initialize glut menus*/
		int trajectmenu = glutCreateMenu( trajectories );
		glutAddMenuEntry( "0, 0, -30", 0 );
		glutAddMenuEntry( "0, -30, 0", 1 );
		glutAddMenuEntry( "-30, 0, 0", 2 );
		glutAddMenuEntry( "Bird's Eye View", 3 );
		glutAddMenuEntry( "other", 4 );

		int leftmenu = glutCreateMenu( left_menu );
		glutAddSubMenu( "Trajectories", trajectmenu );
		glutAttachMenu( GLUT_LEFT_BUTTON );

		int rightmenu = glutCreateMenu( right_menu );
		glutAddMenuEntry( "Exit", 0 );
		glutAttachMenu( GLUT_RIGHT_BUTTON );

		/*Initialize the rest of the program*/
		m_camera.eye = Vec3( 0.f, 0.f, 30 );
		m_camera.at = Vec3( 0.f, 0.f, 0.f );
		m_camera.up = Vec3( 0.f, 1.f, 0.f );

		InitializeLists();

		for( unsigned u = 0; u < 30; ++u )
			m_fish.push_back( Fish( m_board.LowerBounds, m_board.UpperBounds ) );

		for( unsigned u = 0; u < 30; ++u )
			m_waterbugs.push_back( WaterBug( m_board.LowerBounds, m_board.UpperBounds_Floor ) );

		for( unsigned u = 0; u < 100; ++u )
			m_particles.push_back( Particle( m_board.LowerBounds, m_board.UpperBounds ) );
	

		/*run the glut mainloop*/
		glutMainLoop();
	}
};
static Program glprogram; //thought it's global, it's a necessary evil because of glut

int main( int argc, char ** argv )
{
	glprogram.RunProgram( argc, argv );
}

void Program::DisplayFunc()
{
	//glprogram.Advance();
}
void Program::ReshapeFunc( int Width, int Height )
{
	glprogram.ReshapeWindow( Width, Height );
}
void Program::MouseFunc( int Button, int State, int posx, int posy )
{
	glprogram.MouseEvent( Button, State, posx, posy );
}
void Program::KeyboardFunc( unsigned char key, int, int )
{
	glprogram.KeyboardEvent( key );
}
void Program::SpecialFunc( int Key, int, int )
{
	glprogram.SpecialKey( Key );
}
void Program::TimerFunc( int Val )
{
	glprogram.Advance();
	glutTimerFunc( 16, &TimerFunc, 0 );
}
void Program::IdleFunc()
{
	glutPostRedisplay();
}

void Program::trajectories( int menuitem )
{
	switch( menuitem )
	{
	case 0:
		glprogram.SetPreTrajectory( Vec3( 0.f, 0.f, 0.f ), Vec3( 0.f, 0.f, 30.f ) );
		break;

	case 1:
		glprogram.SetPreTrajectory( Vec3( 0.f, 0.f, 0.f ), Vec3( 0.1f, 30.0f, 0.f ) );
		break;

	case 2:
		glprogram.SetPreTrajectory( Vec3( 0.f, 0.f, 0.f ), Vec3( 30.f, 0.f, 0.f ) );
		break;

	case 3:
		glprogram.SetBirdsEyeViewTrajectory();
		break;

	case 4:
		glprogram.SetCustomTrajectory();
		break;
	}
}
void Program::left_menu( int menuitem )
{
	switch( menuitem )
	{
	case 0:;
	default:;
	}
}
void Program::right_menu( int menuitem )
{
	switch( menuitem )
	{
	case 0:
		puts( "Goodbye" );
		glutDestroyWindow( glprogram.WindowId );
		break;
	}
}