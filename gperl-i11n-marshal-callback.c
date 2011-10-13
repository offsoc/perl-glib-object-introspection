/* -*- mode: c; indent-tabs-mode: t; c-basic-offset: 8; -*- */

static gpointer
sv_to_callback (GIArgInfo * arg_info,
                GITypeInfo * type_info,
                SV * sv,
                GPerlI11nInvocationInfo * invocation_info)
{
	GPerlI11nPerlCallbackInfo *callback_info;
	GIScopeType scope;

	/* the destroy notify func is handled by handle_automatic_arg */

	dwarn ("      Perl callback at %d (%s)\n",
	       invocation_info->current_pos,
	       g_base_info_get_name (arg_info));

	callback_info = create_perl_callback_closure (type_info, sv);
	callback_info->data_pos = g_arg_info_get_closure (arg_info);
	callback_info->destroy_pos = g_arg_info_get_destroy (arg_info);
	callback_info->free_after_use = FALSE;

	dwarn ("      Perl callback data at %d, destroy at %d\n",
	       callback_info->data_pos, callback_info->destroy_pos);

	scope = (sv == &PL_sv_undef)
		? GI_SCOPE_TYPE_CALL
		: g_arg_info_get_scope (arg_info);
	switch (scope) {
	    case GI_SCOPE_TYPE_CALL:
		dwarn ("      Perl callback has scope 'call'\n");
		invocation_info->free_after_call
			= g_slist_prepend (invocation_info->free_after_call,
			                   callback_info);
		break;
	    case GI_SCOPE_TYPE_NOTIFIED:
		dwarn ("      Perl callback has scope 'notified'\n");
		/* This case is already taken care of by the notify
		 * stuff above */
		break;
	    case GI_SCOPE_TYPE_ASYNC:
		dwarn ("      Perl callback has scope 'async'\n");
		/* FIXME: callback_info->free_after_use = TRUE; */
		break;
	    default:
		ccroak ("unhandled scope type %d encountered",
		       g_arg_info_get_scope (arg_info));
	}

	invocation_info->callback_infos =
		g_slist_prepend (invocation_info->callback_infos,
		                 callback_info);

	dwarn ("      returning Perl closure %p from info %p\n",
	       callback_info->closure, callback_info);
	return callback_info->closure;
}

static gpointer
sv_to_callback_data (SV * sv,
                     GPerlI11nInvocationInfo * invocation_info)
{
	GSList *l;
	if (!invocation_info)
		return NULL;
	for (l = invocation_info->callback_infos; l != NULL; l = l->next) {
		GPerlI11nPerlCallbackInfo *callback_info = l->data;
		if (callback_info->data_pos == invocation_info->current_pos) {
			dwarn ("      user data for Perl callback %p\n",
			       callback_info);
			attach_perl_callback_data (callback_info, sv);
			return callback_info;
		}
	}
	if (invocation_info->is_callback) {
		GPerlI11nCCallbackInfo *wrapper = INT2PTR (GPerlI11nCCallbackInfo*, SvIV (sv));
		dwarn ("      user data for C callback %p\n", wrapper);
		return wrapper->data;
	}
	return NULL;
}

static SV *
callback_to_sv (GICallableInfo *interface, gpointer func, GPerlI11nInvocationInfo *invocation_info)
{
	GIArgInfo *arg_info;
	GPerlI11nCCallbackInfo *callback_info;
	HV *stash;
	SV *code_sv, *data_sv;
	GIScopeType scope;

	GSList *l;
	for (l = invocation_info->callback_infos; l != NULL; l = l->next) {
		GPerlI11nCCallbackInfo *callback_info = l->data;
		if (invocation_info->current_pos == callback_info->destroy_pos) {
			dwarn ("      destroy notify for C callback %p\n",
			       callback_info);
			callback_info->destroy = func;
			/* release_c_callback is called from
			 * Glib::Object::Introspection::_FuncWrapper::DESTROY */
			return NULL;
		}
	}

	arg_info = g_callable_info_get_arg (invocation_info->interface,
	                                    invocation_info->current_pos);

	dwarn ("      C callback at %d (%s)\n",
	       invocation_info->current_pos,
	       g_base_info_get_name (arg_info));

	callback_info = create_c_callback_closure (interface, func);
	callback_info->data_pos = g_arg_info_get_closure (arg_info);
	callback_info->destroy_pos = g_arg_info_get_destroy (arg_info);
	callback_info->free_after_use = FALSE;

	if (func) {
		data_sv = newSViv (PTR2IV (callback_info));
		stash = gv_stashpv ("Glib::Object::Introspection::_FuncWrapper", TRUE);
		code_sv = sv_bless (newRV_noinc (data_sv), stash);
	} else {
		data_sv = code_sv = &PL_sv_undef;
	}
	callback_info->data_sv = data_sv;

	dwarn ("      C callback data at %d, destroy at %d\n",
	       callback_info->data_pos, callback_info->destroy_pos);

	scope = func
		? g_arg_info_get_scope (arg_info)
		: GI_SCOPE_TYPE_CALL;
	switch (scope) {
	    case GI_SCOPE_TYPE_CALL:
		dwarn ("      C callback has scope 'call'\n");
		invocation_info->free_after_call
			= g_slist_prepend (invocation_info->free_after_call,
			                   callback_info);
		break;
	    case GI_SCOPE_TYPE_NOTIFIED:
		dwarn ("      C callback has scope 'notified'\n");
		/* This case is already taken care of by the notify
		 * stuff above */
		break;
	    case GI_SCOPE_TYPE_ASYNC:
		dwarn ("      C callback has scope 'async'\n");
		/* FIXME: callback_info->free_after_use = TRUE; */
		break;
	    default:
		ccroak ("unhandled scope type %d encountered",
		       g_arg_info_get_scope (arg_info));
	}

	g_base_info_unref (arg_info);

	invocation_info->callback_infos =
		g_slist_prepend (invocation_info->callback_infos,
		                 callback_info);

	dwarn ("      returning C closure %p from info %p\n",
	       code_sv, callback_info);
	return code_sv;
}

static SV *
callback_data_to_sv (gpointer data,
                     GPerlI11nInvocationInfo * invocation_info)
{
	GSList *l;
	if (!data)
		return NULL;
	if (!invocation_info)
		return NULL;
	for (l = invocation_info->callback_infos; l != NULL; l = l->next) {
		GPerlI11nCCallbackInfo *callback_info = l->data;
		if (callback_info->data_pos == invocation_info->current_pos) {
			dwarn ("      user data for C callback %p\n",
			       callback_info);
			attach_c_callback_data (callback_info, data);
			return callback_info->data_sv;
		}
	}
	if (invocation_info->is_callback) {
		GPerlI11nPerlCallbackInfo *wrapper = data;
		dwarn ("      user data for Perl callback %p\n", wrapper);
		return wrapper->data;
	}
	return NULL;
}
