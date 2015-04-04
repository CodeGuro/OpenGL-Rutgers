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
	struct Vertex //self explanitory
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
		Board() : m_width( 700 ), m_height( 700 )
		{
		}
	};
	struct Wire
	{
		std::vector< Vertex > vertices;
		std::vector< Vertex > latches; //the 'terminals'
		void AddLine( Vec3 const & pos2, Vec3 const & pos1, Vec3 const & col )
		{
			vertices.push_back( Vertex( pos2, col ) );
			vertices.push_back( Vertex( pos1, col ) );
		}
		/*Use the named constructor idiom*/
		static Wire XY( Vec3 const & pos2, Vec3 const & pos1, Vec3 const & col )
		{
			Wire res;
			res.latches.push_back( pos2 );
			res.AddLine( pos2, Vec3( pos1.x, pos2.y, 0.f ), col );
			res.AddLine( res.vertices.back().position, pos1, col );
			res.latches.push_back( pos1 );
			return res;
		}
		static Wire XYX( Vec3 const & pos2, Vec3 const & pos1, Vec3 const & col, float frac )
		{
			Wire res;
			res.latches.push_back( pos2 );
			res.AddLine( pos2, Vec3( pos1.x + (1.f - frac) * (pos2.x - pos1.x), pos2.y, 0.f  ), col );
			res.AddLine( res.vertices.back().position, Vec3( res.vertices.back().position.x, pos1.y, 0.f ), col );
			res.AddLine( res.vertices.back().position, pos1, col );
			res.latches.push_back( pos1 );
			return res;
		}
		static Wire YX( Vec3 const & pos2, Vec3 const & pos1, Vec3 const & col )
		{
			Wire res;
			res.latches.push_back( pos2 );
			res.AddLine( pos2, Vec3( pos2.x, pos1.y, 0.f ), col );
			res.AddLine( res.vertices.back().position, pos1, col );
			res.latches.push_back( pos1 );
			return res;
		}
		static Wire YXY( Vec3 const & pos2, Vec3 const & pos1, Vec3 const & col, float frac )
		{
			Wire res;
			res.latches.push_back( pos2 );
			res.AddLine( pos2, Vec3( pos2.x, pos1.y + (1.f - frac) * (pos2.y - pos1.y), 0.f ), col );
			res.AddLine( res.vertices.back().position, Vec3( pos1.x, res.vertices.back().position.y, 0.f ), col );
			res.AddLine( res.vertices.back().position, pos1, col );
			res.latches.push_back( pos1 );
			return res;
		}
		void DrawWire()
		{
			/*Draw the wire*/
			glLineWidth( 2.5f );
			glBegin( GL_LINES );
			for( unsigned u = 0; u < vertices.size(); ++u )
			{
				Vertex & v = vertices[ u ];
				glColor3f( v.color.x, v.color.y, v.color.z );
				glVertex2f( v.position.x, v.position.y );
			}
			glEnd();

			/*Draw the terminal endpoints*/
			glPointSize( 10.f );
			glBegin( GL_POINTS );
			glColor3f( 1.f, 1.f, 1.f );
			for( unsigned u = 0; u < latches.size(); ++u )
				glVertex2f( latches[ u ].position.x, latches[ u ].position.y );
			glEnd();
		}
	};
	struct Logical_Gate
	{
		enum type_gate
		{
			l_or, l_and, l_not
		};
		Vec3 position;
		float rotation;
		std::vector< Vertex > vertices;
		std::vector< Wire > terminalwires;
		float GateSize;
		bool input1;
		bool input2;

		void SetUpTerminal( type_gate gate_type ) //setting up the terminal 'latches'
		{
			terminalwires.clear();
			float offset = GateSize / 3.f * 0.f;

			/*input*/
			if( gate_type != l_not )
			{
				terminalwires.push_back( Wire::XY( Vec3( -GateSize * 1.5f + offset, -GateSize / 2, 0.f ), Vec3( 0.f, 0.f, 0.f ), Vec3( 1.f, 1.f, 1.f ) ) );
				terminalwires.push_back( Wire::XY( Vec3( -GateSize * 1.5f + offset, GateSize / 2, 0.f ), Vec3( 0.f, 0.f, 0.f ), Vec3( 1.f, 1.f, 1.f ) ) );
			}
			else
			{
				terminalwires.push_back( Wire::XY( Vec3( -GateSize * 1.5f + offset, 0.f, 0.f ), Vec3( 0.f, 0.f, 0.f ), Vec3( 1.f, 1.f, 1.f ) ) );
				input2 = false;
			}
			/*output*/
			terminalwires.push_back( Wire::XY( Vec3( 0.f, 0.f, 0.f ), Vec3( GateSize * 1.5f + offset, 0.f, 0.f ), Vec3( 1.f, 1.f, 1.f ) ) );
		}
		void MakeAndGate() //half-circle
		{
			vertices.clear();
			SetUpTerminal( l_and );
			unsigned const sides = 6;
			float pi = 2.f * acos( 0.f );
			float ang = pi / 2.f;
			for( unsigned u = 0; u < sides; ++u, ang += 2.f * pi / sides )
				vertices.push_back( Vertex( Vec3( GateSize * cos( ang ), GateSize * sin( ang ), 0.f ) , Vec3( 1.f, 0.f, 0.f ) ) );
		}
		void MakeOrGate()
		{
			vertices.clear();
			SetUpTerminal( l_or );
			unsigned const sides = 32;
			float pi = 2.f * acos( 0.f );
			float ang = pi / 2;
			for( unsigned u = 0; u < sides + 1 ; ++u, ang -= pi / sides )
				vertices.push_back( Vertex( Vec3( GateSize / -2 + GateSize * cos( ang ), GateSize * sin( ang ), 0.f ) , Vec3( 1.f, 1.f, 0.f ) ) );
		}
		void MakeNotGate()
		{
			vertices.clear();
			SetUpTerminal( l_not );
			unsigned const sides = 3;
			float pi = 2.f * acos( 0.f );
			float ang = 0;
			for( unsigned u = 0; u < sides; ++u, ang -= 2.f * pi / 3 )
				vertices.push_back( Vertex( Vec3( GateSize * cos( ang ), GateSize * sin( ang ), 0.f ) , Vec3( 1.f, 1.f, 1.f ) ) );
		}
		void DrawGate() //draw the gate
		{
			glPushMatrix();
			glLoadIdentity();
			glTranslatef( position.x, position.y, 0.f );
			glRotatef( rotation, 0, 0, 1 );

			/*Draw the wires first, so the gates will be on top*/
			for( unsigned u = 0; u < terminalwires.size(); ++u )
				terminalwires[ u ].DrawWire();

			/*Now draw the gates*/
			glBegin( GL_POLYGON );
			for( unsigned u = 0; u < vertices.size(); ++u )
			{
				Vertex & v = vertices [ u ];
				glColor3f( v.color.x, v.color.y, v.color.z );
				glVertex2f( v.position.x, v.position.y );
			}
			glEnd();
			glPopMatrix();
		}
		void CloseInput( int input ) //input is cut off
		{
			if( input == 1 ) input1 = false;
			else if( input == 2 ) input2 = false;
		}
		bool TestSetInputLatch( int input, Vec3 const & click , Vec3 * out ) //returns false if already connected or not clicked on
		{
			if( !(input == 1 || input == 2) )
				return false;
			bool res = ( input == 1 ? input1 : input2 );
			Vec3 const & posg = ( terminalwires.begin() + input - 1 )->latches.front().position;
			Vec3 pos = posg;
			pos.x += position.x, pos.y += position.y, pos.z += position.z;
			if( !( click.x > pos.x + 5.f || click.x < pos.x - 5.f //collision test
				|| click.y > pos.y + 5.f || click.y < pos.y - 5.f ) )
			{
				if( !res )
					return false;
				CloseInput( input );
				if( res ) printf( "input terminal %i selected\n", input );
				if( out ) *out = pos;
				return true;
			}
			return false;
		}
		bool TestSetOutputLatch( Vec3 const & click, Vec3 * out ) //returns true if clicked on, no matter if connected
		{
			Vec3 const & posg = terminalwires.back().latches.back().position;
			Vec3 pos = posg;
			pos.x += position.x, pos.y += position.y, pos.z += position.z;
			if( !( click.x > pos.x + 5.f || click.x < pos.x - 5.f //collision test
				|| click.y > pos.y + 5.f || click.y < pos.y - 5.f ) )
			{
				puts( "Output terminal selected" );
				if( out ) *out = pos;
				return true;
			}
			return false;
		}

		Logical_Gate( float GateSize ) : GateSize( GateSize ), input1( true ), input2( true )
		{
		}
	};
	class Selection
	{
	public:
		enum type_gate
		{
			l_or, l_and, l_not
		};
		enum type_wire
		{
			w_xy, w_xyx, w_yx, w_yxy
		};

		Selection() : gate_type( l_or ), wire_type( w_xy ), select_input( false ), color( 1.f, 0.f, 0.f ) //defaults
		{
		}
		void SelectGate( type_gate type )
		{
			gate_type = type;
		}
		void SelectWire( int menuitem )
		{
			frac = 0.f;
			switch( menuitem )
			{
			case 0:
				wire_type = w_xy;
				puts( "Selected \"XY\" wire" );
				break;
			case 1:
				wire_type = w_xyx;
				puts( "Type fraction in decimal form, then press \"Enter\"" );
				break;
			case 2:
				wire_type = w_yx;
				puts( "Selected \"YX\" wire" );
				break;
			case 3:
				wire_type = w_yxy;
				puts( "Type fraction in decimal form, then press \"Enter\"" );
				break;	
			}
		}
		void SelectColor( int menuitem )
		{
			switch( menuitem )
			{
			case 0:
				color = Vec3( 1.f, 0.f, 0.f );
				puts( "Selected \"Red\"" );
				break;
			case 1:
				color = Vec3( 1.f, 1.f, 0.f );
				puts( "Selected \"Yellow\"" );
				break;
			case 2:
				color = Vec3( 0.f, 1.f, 0.f );
				puts( "Selected \"Green\"" );
				break;
			case 3:
				color = Vec3( 0.f, 1.f, 1.f );
				puts( "Selected \"Cyan\"" );
				break;
			case 4:
				color = Vec3( 0.f, 0.f, 1.f );
				puts( "Selected \"Blue\"" );
				break;
			}
		}
		bool GateSelectTest( std::vector< Logical_Gate > const & vgates, Vec3 const & click )
		{
			/*check if it's one of the 3 (selection) gates*/
			unsigned u = 0;
			do
			{
				Logical_Gate const & gate = vgates[ u ];
				if( !( click.x > gate.position.x + gate.GateSize || click.x < gate.position.x - gate.GateSize /*collision detection*/
					|| click.y > gate.position.y + gate.GateSize || click.y < gate.position.y - gate.GateSize ) )
					break;
			} while( ++u < 3 );

			switch( u )
			{
			case 0:
				puts( "Selected \"OR\"" );
				SelectGate( l_or );
				return true;
			case 1:
				puts( "Selected \"AND\"" );
				SelectGate( l_and );
				return true;
			case 2:
				puts( "Selected \"NOT\"" );
				SelectGate( l_not );
				return true;
			default:
				return false;
			}
		}
		void PlaceGate( std::vector< Logical_Gate > & vgates, Vec3 const & click )
		{
			for( unsigned u = 0; u < vgates.size(); ++u )
			{
				Logical_Gate const & gate = vgates[ u ];
				if( !( click.x > gate.position.x + gate.GateSize * 3.5 || click.x < gate.position.x - gate.GateSize * 3.5 /*Collision detection*/
					|| click.y > gate.position.y + gate.GateSize * 2.5 || click.y < gate.position.y - gate.GateSize * 2.5 ) )
				{
					//collision test, do not place gate here if it's too close to another gate
					puts( "must place gate further away from this gate" );
					return;
				}
			}
			Logical_Gate gate( 20.f );
			switch( gate_type )
			{
			case l_and:
				gate.MakeAndGate();
				break;
			case l_or:
				gate.MakeOrGate();
				break;
			case l_not:
				gate.MakeNotGate();
				break;
			default:
				return;
			}
			gate.position = click;
			gate.rotation = 0.f;
			vgates.push_back( gate );
			puts( "Gate place successfully" );
		}
		void PlaceWire( std::vector< Wire > & vwires )
		{
			Wire wire;
			switch( wire_type )
			{
			case w_xy:
				wire = Wire::XY( posout, posin, color );
				break;
			case w_xyx:
				wire = Wire::XYX( posout, posin, color, frac );
				break;
			case w_yx:
				wire = Wire::YX( posout, posin, color );
				break;
			case w_yxy:
				wire = Wire::YXY( posout, posin, color, frac );
				break;
			default:
					return;
			}
			vwires.push_back( wire );
		}
		void MakeFrac()
		{
			if( frac_chars.size() )
			{
				frac = (float)atof( frac_chars.c_str() );
				frac_chars.clear();
				if( frac <= 0.f || frac >= 1.f )
				{
					frac = 0.f;
					puts( "\nFraction must be between 0 and 1, exclusively" );
				}
				else
					printf( "%s%f\n", "\nWire selected with fraction: ", frac );
			}
			else
				puts( "Buffer empty" );
		}
		void AddCharFrac( unsigned char value ) /*i.e. '0', '.', '5'*/
		{
			printf( "%c", value );
			frac_chars.push_back( value );
		}
		bool SelectingInput()
		{
			return select_input;
		}
		void UpdateLatchSelection( Vec3 const & pos, std::vector< Wire > & vwires ) /*middle-click event*/
		{
			( (select_input ) ? posin : posout ) = pos;
			if( select_input ) 
				PlaceWire( vwires );
			select_input = !select_input;
		}
	private:
		type_gate gate_type;
		type_wire wire_type;
		bool select_input;
		Vec3 color;
		Vec3 posin;
		Vec3 posout;
		float frac;
		std::string frac_chars;
	};

	int WindowId;
	Board m_board;
	std::vector< Wire > m_wires;
	std::vector< Logical_Gate > m_gates;
	Selection m_selections;

	static void DisplayFunc();
	static void ReshapeFunc( int Width, int Height );
	static void MouseFunc( int Button, int State, int posx, int posy );
	static void KeyboardFunc( unsigned char Key, int, int );
	static void SpecialFunc( int Key, int, int );
	static void IdleFunc();

	static void wire_shape( int menuitem );
	static void colors( int menuitem );
	static void right_menu( int menuitem );

	void Advance() /*mostly drawing*/
	{
		glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
		glClear( GL_COLOR_BUFFER_BIT );

		for( unsigned u = 0; u < m_gates.size(); ++u )
			m_gates[ u ].DrawGate();

		for( unsigned u = 0; u < m_wires.size(); ++u )
			m_wires[ u ].DrawWire();

		glFlush();
		glutSwapBuffers();
	}
	void clear() /*self explanatory*/
	{
		puts( "Clearing gates" );
		m_gates.erase( m_gates.begin() + 3, m_gates.end() );
		puts( "Clearing wires" );
		m_wires.clear();
	}
	void ReshapeWindow( int Width, int Height )
	{
		m_board.m_width = Width;
		m_board.m_height = Height;
		glViewport( 0, 0, Width, Height );
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		gluOrtho2D( 0.0f, (GLdouble)Width, (GLdouble)Height, 0.0f );
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
	}
	void MouseEvent( int Button, int State, int Xpos, int Ypos )
	{
		Vec3 click( (float)Xpos, (float)Ypos, 0.f );
		if( Button == GLUT_LEFT_BUTTON && State == GLUT_DOWN ) 
		{
			/*left click*/
			if( m_selections.GateSelectTest( m_gates, click ) );
			else if( Ypos > 100 ) /*test if Y is greater than 100 (avoid placing gates near the selectors)*/
				m_selections.PlaceGate( m_gates, click );
			else puts( "Place a gate lower" );
		}
		else if( Button == GLUT_MIDDLE_BUTTON && State == GLUT_DOWN )
		{
			/*middle button*/
			/*input/output terminal selection*/
			for( unsigned u = 3; u < m_gates.size(); ++u ) /*first 3 gates are selector gates, do not test against them*/
			{
				Logical_Gate & gate = m_gates[ u ];
				Vec3 place;
				
				if( m_selections.SelectingInput() ) /*input1 selected*/
				{
					if( gate.TestSetInputLatch( 1, click, &place ) )
					{
						m_selections.UpdateLatchSelection( place, m_wires );
						return;
					}
					else if( gate.TestSetInputLatch( 2, click, &place ) ) /*input2 selected*/
					{
						m_selections.UpdateLatchSelection( place, m_wires );
						return;
					}
				}
				else if( gate.TestSetOutputLatch( click, &place ) ) /*output selected*/
				{
					m_selections.UpdateLatchSelection( place, m_wires );
					return;
				}
			}
			printf( "%s%s%s\n", "Invalid ", m_selections.SelectingInput() ? "Input" : "Output", " terminal" ); /*either terminal not selected, or the wrong end got selected*/
			return;
		}
		else if( Button == GLUT_RIGHT_BUTTON && State == GLUT_DOWN )
		{
			/*right button*/
			/*we don't really need this, menu items are processed by glut for us*/
		}
		glutPostRedisplay();
	}
	void KeyboardEvent( unsigned char Key )
	{
		switch( Key )
		{
		/*Escape / Quit*/
		case 27:
		case 'q':
			glutDestroyWindow( WindowId );
			break;
		case 13:
			m_selections.MakeFrac();
			break;
		default:
			m_selections.AddCharFrac( Key );
		}
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
		WindowId = glutCreateWindow( "Assignment 2" );
		glutDisplayFunc( &DisplayFunc );
		glutMouseFunc( &MouseFunc );
		glutKeyboardFunc( &KeyboardFunc );
		glutSpecialFunc( &SpecialFunc );
		glutReshapeFunc( &ReshapeFunc );
		glutIdleFunc( &IdleFunc );

		/*Initialize glut menus*/
		int wishape = glutCreateMenu( wire_shape );
		glutAddMenuEntry( "XY", 0 );
		glutAddMenuEntry( "XYX", 1 );
		glutAddMenuEntry( "YX", 2 );
		glutAddMenuEntry( "YXY", 3 );
		int cols = glutCreateMenu( colors );
		glutAddMenuEntry( "Red", 0 );
		glutAddMenuEntry( "Yellow", 1 );
		glutAddMenuEntry( "Green", 2 );
		glutAddMenuEntry( "Cyan", 3 );
		glutAddMenuEntry( "Blue", 4 );
		int rightmenu = glutCreateMenu( right_menu );
		glutAddSubMenu( "Wire shape", wishape );
		glutAddSubMenu( "Colors", cols );
		glutAddMenuEntry( "Clear", 0 );
		glutAddMenuEntry( "Exit", 1 );
		glutAttachMenu(GLUT_RIGHT_BUTTON);

		/*Initialize the (selection) gates*/
		for( unsigned u = 0; u < 3; ++u )
		{
			Logical_Gate gate( 20.f );
			u == 0 ? gate.MakeOrGate() : u == 1 ? gate.MakeAndGate() : gate.MakeNotGate();
			gate.position = Vec3( gate.GateSize * 2  +  gate.GateSize * 4.f * u, 50.f, 0.f );
			gate.rotation = 0.f;
			m_gates.push_back( gate );
		}

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
	glprogram.Advance();
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
void Program::IdleFunc()
{
	glutPostRedisplay();
}

void Program::wire_shape( int menuitem )
{
	glprogram.m_selections.SelectWire( menuitem );
}
void Program::colors( int menuitem )
{
	glprogram.m_selections.SelectColor( menuitem );
}
void Program::right_menu( int menuitem )
{
	switch( menuitem )
	{
	case 0:
		glprogram.clear();
		break;
	case 1:
		puts( "Goodbye" );
		glutDestroyWindow( glprogram.WindowId );
		break;
	}
}