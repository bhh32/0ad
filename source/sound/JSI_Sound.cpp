#include "precompiled.h"
#include "JSI_Sound.h"
#include "Vector3D.h"

#include "lib/res/sound/snd_mgr.h"
#include "lib/res/h_mgr.h"	// h_filename


JSI_Sound::JSI_Sound( const CStr& Filename )
{
	const char* fn = Filename.c_str();
	m_Handle = snd_open( fn );

	// special-case to avoid throwing exceptions if quickstart has
	// disabled sound: set a flag queried by Construct; the object will
	// then immediately be freed.
	if(m_Handle == ERR_AGAIN)
	{
		m_SoundDisabled = true;
		return;
	}
	m_SoundDisabled = false;

	// if open failed, raise an exception - it's the only way to
	// report errors, since we're in the ctor and don't want to move
	// the open call elsewhere (by requiring an explicit open() call).
	THROW_ERR(m_Handle);	// caught by JSI_Sound::Construct.

	snd_set_pos( m_Handle, 0,0,0, true);
}

JSI_Sound::~JSI_Sound()
{
	this->Free(0, 0, 0);
}


bool JSI_Sound::SetGain( JSContext* cx, uintN argc, jsval* argv )
{
	debug_assert( argc >= 1 );
	float gain;
	if( !ToPrimitive<float>( cx, argv[0], gain) )
		return false;

	snd_set_gain(m_Handle, gain);
	return true;
}

bool JSI_Sound::SetPitch( JSContext* cx, uintN argc, jsval* argv )
{
	debug_assert( argc >= 1 );
	float pitch;
	if( !ToPrimitive<float>( cx, argv[0], pitch) )
		return false;

	snd_set_pitch(m_Handle, pitch);
	return true;
}

bool JSI_Sound::SetPosition( JSContext* cx, uintN argc, jsval* argv )
{
	debug_assert( argc >= 1 );
	CVector3D pos;
	// absolute world coords
	if( ToPrimitive<CVector3D>( cx, argv[0], pos ) )
		snd_set_pos(m_Handle, pos[0], pos[1], pos[2]);
	// relative, 0 offset - right on top of the listener
	// (we don't need displacement from the listener, e.g. always behind)
	else
		snd_set_pos(m_Handle, 0,0,0, true);

	return true;
}


bool JSI_Sound::Fade( JSContext* cx, uintN argc, jsval* argv )
{
	debug_assert( argc >= 3 );
	float initial_gain, final_gain;
	float length;
	if( !ToPrimitive<float>( cx, argv[0], initial_gain) )
		return false;
	if( !ToPrimitive<float>( cx, argv[1], final_gain) )
		return false;
	if( !ToPrimitive<float>( cx, argv[2], length) )
		return false;
	snd_fade(m_Handle, initial_gain, final_gain, length, FT_S_CURVE);

	// HACK: snd_fade causes <m_Handle> to be automatically freed when a
	// fade to 0 has completed. however, we're still holding on to a
	// reference, which will cause a double-free warning when Free() is
	// called from the dtor. solution is to neuter our Handle by
	// setting it to 0 (ok since it'll be freed). this does mean that
	// no further operations can be carried out during that final fade.
	m_Handle = 0;

	return true;
}

// start playing the sound (one-shot).
// it will automatically be freed when done.
bool JSI_Sound::Play( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	snd_play(m_Handle);
	return true;
}

// request the sound be played until free() is called. returns immediately.
bool JSI_Sound::Loop( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	snd_set_loop(m_Handle, true);
	snd_play(m_Handle);
	return true;
}

// stop sound if currently playing and free resources.
// doesn't need to be called unless played via loop() -
// sounds are freed automatically when done playing.
bool JSI_Sound::Free( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	snd_free(m_Handle);	// resets it to 0
	return true;
}


// Script-bound functions


void JSI_Sound::ScriptingInit()
{
	AddMethod<jsval, &JSI_Sound::ToString>( "toString", 0 );
	AddMethod<bool, &JSI_Sound::Play>( "play", 0 );
	AddMethod<bool, &JSI_Sound::Loop>( "loop", 0 );
	AddMethod<bool, &JSI_Sound::Free>( "free", 0 );
	AddMethod<bool, &JSI_Sound::SetGain>( "setGain", 0 );
	AddMethod<bool, &JSI_Sound::SetPitch>( "setPitch", 0 );
	AddMethod<bool, &JSI_Sound::SetPosition>( "setPosition", 0 );
	AddMethod<bool, &JSI_Sound::Fade>( "fade", 0 );

	CJSObject<JSI_Sound>::ScriptingInit( "Sound", &JSI_Sound::Construct, 1 );
}

jsval JSI_Sound::ToString( JSContext* cx, uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	const char* Filename = h_filename(m_Handle);
	wchar_t buffer[256];
	swprintf( buffer, 256, L"[object Sound: %hs]", Filename );
	buffer[255] = 0;
	utf16string str16(buffer, buffer+wcslen(buffer));
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}

JSBool JSI_Sound::Construct( JSContext* cx, JSObject* UNUSED(obj), uint argc, jsval* argv, jsval* rval )
{
	debug_assert( argc >= 1 );
	CStrW filename;
	if( !ToPrimitive<CStrW>( cx, argv[0], filename ) )
		return( JS_FALSE );

	try
	{
		JSI_Sound* newObject = new JSI_Sound( filename );

		// somewhat of a hack to avoid throwing exceptions if
		// sound was disabled due to quickstart. see JSI_Sound().
		if( newObject->m_SoundDisabled )
		{
			delete newObject;
			*rval = JSVAL_NULL;
			return( JS_TRUE );
		}

		newObject->m_EngineOwned = false;
		*rval = OBJECT_TO_JSVAL( newObject->GetScript() );
	}
	catch(int)
	{
		// failed, but this can happen easily enough that we don't want to
		// return JS_FALSE (since that stops the script).
		*rval = JSVAL_NULL;
	}

	return( JS_TRUE );
}
