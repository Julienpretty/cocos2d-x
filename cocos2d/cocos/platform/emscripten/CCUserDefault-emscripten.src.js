/* CCUserDefault-emscripten.src.js */ /* UTF8-Côôkie */

/*
About 'use strict' here:

	We're being run by emscripten_run_script(), which, according to the docs:
		https://kripken.github.io/emscripten-site/docs/porting/connecting_cpp_and_javascript/Interacting-with-code.html#interacting-with-code-call-javascript-from-native
		>> ... This effectively runs the specified JavaScript code from C/C++ using eval().

	As per:
		https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Strict_mode
		>> ... eval code, ..., and the like are entire scripts, and invoking strict mode in them works as expected.
*/
'use strict';

Module.cocos_UserDefault = (function()
{
	const _storage = (function()
	{
		// Storage availability detection stolen from native's app.js. Comments stripped.
		try
		{
			if(
				!(window.localStorage instanceof Storage) ||
				window.localStorage.getItem('dummy') === {}
			)
				throw null;

			return window.localStorage;
		}
		catch(e)
		{
			try
			{
				if(
					!(window.sessionStorage instanceof Storage) ||
					window.sessionStorage.getItem('dummy') === {}
				)
					throw null;

				return window.sessionStorage;
			}
			catch(e)
			{
				const memFallback = {};

				return {

					setItem		:	function(key, value)
					{
						memFallback[key] = value;
					},

					getItem		:	function(key)
					{
						if(!memFallback.hasOwnProperty(key))
							return null;

						return memFallback[key];
					},

					removeItem	:	function(key)
					{
						if(memFallback.hasOwnProperty(key))
							delete memFallback[key];
					}
				};
			}
		}

	})();


	// Since we may share the *storage with other modules, or with emscripten itself, always prefix the keys

	function	_set(key, value)
	{
		try
		{
			_storage.setItem('CCUserDefault::'+key, value);
		}
		catch(e)
		{
		}
	}

	function	_get(key)
	{
		try
		{
			return _storage.getItem('CCUserDefault::'+key);
		}
		catch(e)
		{
			return null;
		}
	}

	function	_remove(key)
	{
		try
		{
			_storage.removeItem('CCUserDefault::'+key);
		}
		catch(e)
		{
		}
	}

	return Object.freeze({

		setValue:	function(key_ptr, value_ptr)
		{
			_set(UTF8ToString(key_ptr), UTF8ToString(value_ptr));
		},

		getValue:	function(key_ptr)
		{
			// We return 0 if the key was not found, a pointer to a null-terminated string otherwise
			// (since we assume we're storing null-terminated base64-encoded data)
			const value = _get(UTF8ToString(key_ptr));

			if(value === null)
				return 0;

			// Since we store base64-encoded strings, the stringToUTF8() output should equal key.length
			// (no multi-byte characters expected), yet always invoke allocateUTF8()...

			return allocateUTF8(value);
		},

		removeValue:	function(key_ptr)
		{
			_remove(UTF8ToString(key_ptr));
		}
	});

})();

