static SV *
struct_to_sv (GIBaseInfo* info,
              GIInfoType info_type,
              gpointer pointer,
              gboolean own)
{
	HV *hv;

	dwarn ("%s: pointer %p\n", G_STRFUNC, pointer);

	if (pointer == NULL) {
		return &PL_sv_undef;
	}

	hv = newHV ();

	switch (info_type) {
	    case GI_INFO_TYPE_BOXED:
	    case GI_INFO_TYPE_STRUCT:
	    {
		gint i, n_fields =
			g_struct_info_get_n_fields ((GIStructInfo *) info);
		for (i = 0; i < n_fields; i++) {
			GIFieldInfo *field_info;
			SV *sv;
			field_info =
				g_struct_info_get_field ((GIStructInfo *) info, i);
			/* FIXME: Check GIFieldInfoFlags. */
			/* FIXME: Is it right to use GI_TRANSFER_NOTHING
			 * here? */
			sv = get_field (field_info, pointer,
			                GI_TRANSFER_NOTHING);
			if (gperl_sv_is_defined (sv)) {
				const gchar *name;
				name = g_base_info_get_name (
				         (GIBaseInfo *) field_info);
				gperl_hv_take_sv (hv, name, strlen (name), sv);
			}
			g_base_info_unref ((GIBaseInfo *) field_info);
		}
		break;
	    }

	    case GI_INFO_TYPE_UNION:
		ccroak ("%s: unions not handled yet", G_STRFUNC);

	    default:
		ccroak ("%s: unhandled info type %d", G_STRFUNC, info_type);
	}

	if (own) {
		/* FIXME: Is it correct to just call g_free here?  What if the
		 * thing was allocated via GSlice? */
		g_free (pointer);
	}

	return newRV_noinc ((SV *) hv);
}

static gpointer
sv_to_struct (GITransfer transfer,
              GIBaseInfo * info,
              GIInfoType info_type,
              SV * sv)
{
	HV *hv;
	gsize size = 0;
	GITransfer field_transfer;
	gpointer pointer = NULL;

	dwarn ("%s: sv %p\n", G_STRFUNC, sv);

	if (!gperl_sv_is_hash_ref (sv))
		ccroak ("need a hash ref to convert to struct of type %s",
		       g_base_info_get_name (info));
	hv = (HV *) SvRV (sv);

	switch (info_type) {
	    case GI_INFO_TYPE_BOXED:
	    case GI_INFO_TYPE_STRUCT:
		size = g_struct_info_get_size ((GIStructInfo *) info);
		break;
	    case GI_INFO_TYPE_UNION:
		size = g_union_info_get_size ((GIStructInfo *) info);
		break;
	    default:
		g_assert_not_reached ();
	}

	dwarn ("  size: %d\n", size);

	field_transfer = GI_TRANSFER_NOTHING;
	dwarn ("  transfer: %d\n", transfer);
	switch (transfer) {
	    case GI_TRANSFER_EVERYTHING:
		field_transfer = GI_TRANSFER_EVERYTHING;
		/* fall through */
	    case GI_TRANSFER_CONTAINER:
		/* FIXME: What if there's a special allocator for the record?
		 * Like GSlice? */
		pointer = g_malloc0 (size);
		break;

	    default:
		pointer = gperl_alloc_temp (size);
		break;
	}

	switch (info_type) {
	    case GI_INFO_TYPE_BOXED:
	    case GI_INFO_TYPE_STRUCT:
	    {
		gint i, n_fields =
			g_struct_info_get_n_fields ((GIStructInfo *) info);
		for (i = 0; i < n_fields; i++) {
			GIFieldInfo *field_info;
			const gchar *field_name;
			SV **svp;
			field_info = g_struct_info_get_field (
			               (GIStructInfo *) info, i);
			/* FIXME: Check GIFieldInfoFlags. */
			field_name = g_base_info_get_name (
			               (GIBaseInfo *) field_info);
			svp = hv_fetch (hv, field_name, strlen (field_name), 0);
			if (svp && gperl_sv_is_defined (*svp)) {
				set_field (field_info, pointer,
				           field_transfer, *svp);
			}
			g_base_info_unref ((GIBaseInfo *) field_info);
		}
		break;
	    }

	    case GI_INFO_TYPE_UNION:
		ccroak ("%s: unions not handled yet", G_STRFUNC);

	    default:
		ccroak ("%s: unhandled info type %d", G_STRFUNC, info_type);
	}

	return pointer;
}