
// TODO(Faruk): Make columns optional output.
// TODO(Faruk): Put neighbour visits into a function.
// TODO(Faruk): Add debug flag to reduce number of outputs.

#include "../dep/laynii_lib.h"

int show_help(void) {
    printf(
    "LN2_LAYERS: Cortical gray matter layering.\n"
    "\n"
    "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
    "!! BEWARE! WORK IN PROGRESS... USE WITH CAUTION !!\n"
    "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
    "\n"
    "Usage:\n"
    "    LN2_LAYERS -rim rim.nii \n"
    "\n"
    "Options:\n"
    "    -help       : Show this help. \n"
    "    -rim        : Specify input dataset.\n"
    "    -nr_layers  : Number of layers. Default is 3.\n"
    "\n");
    return 0;
}

int main(int argc, char*  argv[]) {
    nifti_image*nii1 = NULL;
    char* fin = NULL;
    uint16_t ac, nr_layers = 3;
    float column_size = 1;
    if (argc < 2) {
        return show_help();   // typing '-help' is sooo much work
    }

    // Process user options
    for (ac = 1; ac < argc; ac++) {
        if (!strncmp(argv[ac], "-h", 2)) {
            return show_help();
        } else if (!strcmp(argv[ac], "-rim")) {
            if (++ac >= argc) {
                fprintf(stderr, "** missing argument for -rim\n");
                return 1;
            }
            fin = argv[ac];
        } else if (!strcmp(argv[ac], "-nr_layers")) {
            if (++ac >= argc) {
                fprintf(stderr, "** missing argument for -nr_layers\n");
            } else {
                nr_layers = atof(argv[ac]);
            }
        } else if (!strcmp(argv[ac], "-column_size")) {
            if (++ac >= argc) {
                fprintf(stderr, "** missing argument for -column_size\n");
            } else {
                column_size = atof(argv[ac]);
            }
        } else {
            fprintf(stderr, "** invalid option, '%s'\n", argv[ac]);
            return 1;
        }
    }

    if (!fin) {
        fprintf(stderr, "** missing option '-rim'\n");
        return 1;
    }

    // Read input dataset, including data
    nii1 = nifti_image_read(fin, 1);
    if (!nii1) {
        fprintf(stderr, "** failed to read NIfTI image from '%s'\n", fin);
        return 2;
    }

    log_welcome("LN_3DGROW_LAYERS");
    log_nifti_descriptives(nii1);

    cout << "  Nr. layers: " << nr_layers << endl;

    // Get dimensions of input
    const uint32_t size_z = nii1->nz;
    const uint32_t size_x = nii1->nx;
    const uint32_t size_y = nii1->ny;

    const uint32_t nr_voxels = size_z * size_y * size_x;

    const float dX = nii1->pixdim[1];
    const float dY = nii1->pixdim[2];
    const float dZ = nii1->pixdim[3];

    // Short diagonals
    const float dia_xy = sqrt(dX * dX + dY * dY);
    const float dia_xz = sqrt(dX * dX + dZ * dZ);
    const float dia_yz = sqrt(dY * dY + dZ * dZ);
    // Long diagonals
    const float dia_xyz = sqrt(dX * dX + dY * dY + dZ * dZ);

    // ========================================================================
    // Fix input datatype issues
    nifti_image* nii_rim = copy_nifti_as_int32(nii1);
    int32_t* nii_rim_data = static_cast<int32_t*>(nii_rim->data);

    // Prepare required nifti images
    nifti_image* innerGM_step = copy_nifti_as_float32(nii_rim);
    float* innerGM_step_data = static_cast<float*>(innerGM_step->data);
    nifti_image* innerGM_dist = copy_nifti_as_float32(nii_rim);
    float* innerGM_dist_data = static_cast<float*>(innerGM_dist->data);

    nifti_image* outerGM_step = copy_nifti_as_float32(nii_rim);
    float* outerGM_step_data = static_cast<float*>(outerGM_step->data);
    nifti_image* outerGM_dist = copy_nifti_as_float32(nii_rim);
    float* outerGM_dist_data = static_cast<float*>(outerGM_dist->data);

    nifti_image* err_dist = copy_nifti_as_float32(nii_rim);
    float* err_dist_data = static_cast<float*>(err_dist->data);

    nifti_image* innerGM_id = copy_nifti_as_int32(nii_rim);
    int32_t* innerGM_id_data = static_cast<int32_t*>(innerGM_id->data);
    nifti_image* outerGM_id = copy_nifti_as_int32(nii_rim);
    int32_t* outerGM_id_data = static_cast<int32_t*>(outerGM_id->data);

    nifti_image* innerGM_prevstep_id = copy_nifti_as_int32(nii_rim);
    int32_t* innerGM_prevstep_id_data = static_cast<int32_t*>(innerGM_prevstep_id->data);
    nifti_image* outerGM_prevstep_id = copy_nifti_as_int32(nii_rim);
    int32_t* outerGM_prevstep_id_data = static_cast<int32_t*>(outerGM_prevstep_id->data);
    nifti_image* normdistdiff = copy_nifti_as_float32(nii_rim);
    float* normdistdiff_data = static_cast<float*>(normdistdiff->data);

    nifti_image* nii_layers  = copy_nifti_as_int32(nii_rim);
    int32_t* nii_layers_data = static_cast<int32_t*>(nii_layers->data);

    nifti_image* nii_columns = copy_nifti_as_int32(nii_rim);
    int32_t* nii_columns_data = static_cast<int32_t*>(nii_columns->data);

    nifti_image* middleGM = copy_nifti_as_int32(nii_rim);
    int32_t* middleGM_data = static_cast<int32_t*>(middleGM->data);
    nifti_image* middleGM_id = copy_nifti_as_int32(nii_rim);
    int32_t* middleGM_id_data = static_cast<int32_t*>(middleGM_id->data);

    nifti_image* hotspots = copy_nifti_as_int32(nii_rim);
    int32_t* hotspots_data = static_cast<int32_t*>(hotspots->data);
    nifti_image* curvature = copy_nifti_as_int32(nii_rim);
    int32_t* curvature_data = static_cast<int32_t*>(curvature->data);

    // Setting zero
    for (uint32_t i = 0; i != nr_voxels; ++i) {
        *(innerGM_step_data + i) = 0;
        *(outerGM_step_data + i) = 0;
        *(err_dist_data + i) = 0;
        *(innerGM_id_data + i) = 0;
        *(outerGM_id_data + i) = 0;
        *(innerGM_prevstep_id_data + i) = 0;
        *(outerGM_prevstep_id_data + i) = 0;
        *(middleGM_data + i) = 0;
        *(normdistdiff_data + i) = 0;
        *(nii_layers_data + i) = 0;
        *(nii_columns_data + i) = 0;
        *(hotspots_data + i) = 0;
        *(curvature_data + i) = 0;
    }

    // ========================================================================
    // Grow from WM
    // ========================================================================
    cout << "  Start growing from inner GM (WM-facing border)..." << endl;

    // Initialize grow volume
    for (uint32_t i = 0; i != nr_voxels; ++i) {
        if (*(nii_rim_data + i) == 2) {  // WM boundary voxels within GM
            *(innerGM_step_data + i) = 1.;
            *(innerGM_dist_data + i) = 0.;
            *(innerGM_id_data + i) = i;
        } else {
            *(innerGM_step_data + i) = 0.;
            *(innerGM_dist_data + i) = 0.;
        }
    }

    uint32_t grow_step = 1, voxel_counter = nr_voxels;
    uint32_t ix, iy, iz, j, k;
    float d;
    while (voxel_counter != 0) {
        voxel_counter = 0;
        for (uint32_t i = 0; i != nr_voxels; ++i) {
            if (*(innerGM_step_data + i) == grow_step) {
                tie(ix, iy, iz) = ind2sub_3D(i, size_x, size_y);
                voxel_counter += 1;

                // ------------------------------------------------------------
                // 1-jump neighbours
                // ------------------------------------------------------------
                if (ix != 0) {
                    j = sub2ind_3D(ix-1, iy, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dX;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x) {
                    j = sub2ind_3D(ix+1, iy, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dX;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != 0) {
                    j = sub2ind_3D(ix, iy-1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dY;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != size_y) {
                    j = sub2ind_3D(ix, iy+1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dY;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iz != 0) {
                    j = sub2ind_3D(ix, iy, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dZ;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iz != size_z) {
                    j = sub2ind_3D(ix, iy, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dZ;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }

                // ------------------------------------------------------------
                // 2-jump neighbours
                // ------------------------------------------------------------
                if (ix != 0 && iy != 0) {
                    j = sub2ind_3D(ix-1, iy-1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xy;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iy != size_y) {
                    j = sub2ind_3D(ix-1, iy+1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xy;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != 0) {
                    j = sub2ind_3D(ix+1, iy-1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xy;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != size_y) {
                    j = sub2ind_3D(ix+1, iy+1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xy;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != 0 && iz != 0) {
                    j = sub2ind_3D(ix, iy-1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_yz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != 0 && iz != size_z) {
                    j = sub2ind_3D(ix, iy-1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_yz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != size_z && iz != 0) {
                    j = sub2ind_3D(ix, iy+1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_yz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != size_z && iz != size_z) {
                    j = sub2ind_3D(ix, iy+1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_yz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iz != 0) {
                    j = sub2ind_3D(ix-1, iy, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iz != 0) {
                    j = sub2ind_3D(ix+1, iy, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iz != size_z) {
                    j = sub2ind_3D(ix-1, iy, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iz != size_z) {
                    j = sub2ind_3D(ix+1, iy, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }

                // ------------------------------------------------------------
                // 3-jump neighbours
                // ------------------------------------------------------------
                if (ix != 0 && iy != 0 && iz != 0) {
                    j = sub2ind_3D(ix-1, iy-1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xyz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iy != 0 && iz != size_z) {
                    j = sub2ind_3D(ix-1, iy-1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xyz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iy != size_y && iz != 0) {
                    j = sub2ind_3D(ix-1, iy+1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xyz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != 0 && iz != 0) {
                    j = sub2ind_3D(ix+1, iy-1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xyz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iy != size_y && iz != size_z) {
                    j = sub2ind_3D(ix-1, iy+1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xyz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != size_y && iz != size_z) {
                    j = sub2ind_3D(ix+1, iy-1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xyz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != size_y && iz != 0) {
                    j = sub2ind_3D(ix+1, iy+1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xyz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != size_y && iz != size_z) {
                    j = sub2ind_3D(ix+1, iy+1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(innerGM_dist_data + i) + dia_xyz;
                        if (d < *(innerGM_dist_data + j)
                            || *(innerGM_dist_data + j) == 0) {
                            *(innerGM_dist_data + j) = d;
                            *(innerGM_step_data + j) = grow_step + 1;
                            *(innerGM_id_data + j) = *(innerGM_id_data + i);
                            *(innerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
            }
        }
        grow_step += 1;
    }
    save_output_nifti(fin, "innerGM_step", innerGM_step, false);
    save_output_nifti(fin, "innerGM_dist", innerGM_dist, false);
    save_output_nifti(fin, "innerGM_id", innerGM_id, false);

    // ========================================================================
    // Grow from CSF
    // ========================================================================
    cout << "  Start growing from outer GM..." << endl;

    for (uint32_t i = 0; i != nr_voxels; ++i) {
        if (*(nii_rim_data + i) == 1) {
            *(outerGM_step_data + i) = 1.;
            *(outerGM_dist_data + i) = 0.;
            *(outerGM_id_data + i) = i;
        } else {
            *(outerGM_step_data + i) = 0.;
            *(outerGM_dist_data + i) = 0.;
        }
    }

    grow_step = 1, voxel_counter = nr_voxels;
    while (voxel_counter != 0) {
        voxel_counter = 0;
        for (uint32_t i = 0; i != nr_voxels; ++i) {
            if (*(outerGM_step_data + i) == grow_step) {
                tie(ix, iy, iz) = ind2sub_3D(i, size_x, size_y);
                voxel_counter += 1;

                // ------------------------------------------------------------
                // 1-jump neighbours
                // ------------------------------------------------------------
                if (ix != 0) {
                    j = sub2ind_3D(ix-1, iy, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dX;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x) {
                    j = sub2ind_3D(ix+1, iy, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dX;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != 0) {
                    j = sub2ind_3D(ix, iy-1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dY;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != size_y) {
                    j = sub2ind_3D(ix, iy+1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dY;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iz != 0) {
                    j = sub2ind_3D(ix, iy, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dZ;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iz != size_z) {
                    j = sub2ind_3D(ix, iy, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dZ;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }

                // ------------------------------------------------------------
                // 2-jump neighbours
                // ------------------------------------------------------------
                if (ix != 0 && iy != 0) {
                    j = sub2ind_3D(ix-1, iy-1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xy;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iy != size_y) {
                    j = sub2ind_3D(ix-1, iy+1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xy;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != 0) {
                    j = sub2ind_3D(ix+1, iy-1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xy;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != size_y) {
                    j = sub2ind_3D(ix+1, iy+1, iz, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xy;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != 0 && iz != 0) {
                    j = sub2ind_3D(ix, iy-1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_yz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != 0 && iz != size_z) {
                    j = sub2ind_3D(ix, iy-1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_yz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != size_z && iz != 0) {
                    j = sub2ind_3D(ix, iy+1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_yz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (iy != size_z && iz != size_z) {
                    j = sub2ind_3D(ix, iy+1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_yz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iz != 0) {
                    j = sub2ind_3D(ix-1, iy, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iz != 0) {
                    j = sub2ind_3D(ix+1, iy, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iz != size_z) {
                    j = sub2ind_3D(ix-1, iy, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iz != size_z) {
                    j = sub2ind_3D(ix+1, iy, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }

                // ------------------------------------------------------------
                // 3-jump neighbours
                // ------------------------------------------------------------
                if (ix != 0 && iy != 0 && iz != 0) {
                    j = sub2ind_3D(ix-1, iy-1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xyz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iy != 0 && iz != size_z) {
                    j = sub2ind_3D(ix-1, iy-1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xyz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iy != size_y && iz != 0) {
                    j = sub2ind_3D(ix-1, iy+1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xyz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != 0 && iz != 0) {
                    j = sub2ind_3D(ix+1, iy-1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xyz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != 0 && iy != size_y && iz != size_z) {
                    j = sub2ind_3D(ix-1, iy+1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xyz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != size_y && iz != size_z) {
                    j = sub2ind_3D(ix+1, iy-1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xyz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != size_y && iz != 0) {
                    j = sub2ind_3D(ix+1, iy+1, iz-1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xyz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
                if (ix != size_x && iy != size_y && iz != size_z) {
                    j = sub2ind_3D(ix+1, iy+1, iz+1, size_x, size_y);
                    if (*(nii_rim_data + j) == 3) {
                        d = *(outerGM_dist_data + i) + dia_xyz;
                        if (d < *(outerGM_dist_data + j)
                            || *(outerGM_dist_data + j) == 0) {
                            *(outerGM_dist_data + j) = d;
                            *(outerGM_step_data + j) = grow_step + 1;
                            *(outerGM_id_data + j) = *(outerGM_id_data + i);
                            *(outerGM_prevstep_id_data + j) = i;
                        }
                    }
                }
            }
        }
        grow_step += 1;
    }
    save_output_nifti(fin, "outerGM_step", outerGM_step, false);
    save_output_nifti(fin, "outerGM_dist", outerGM_dist, false);
    save_output_nifti(fin, "outerGM_id", outerGM_id, false);

    // ========================================================================
    // Layers
    // ========================================================================
    cout << "  Start doing layers..." << endl;
    float x, y, z, wm_x, wm_y, wm_z, gm_x, gm_y, gm_z;

    for (uint32_t i = 0; i != nr_voxels; ++i) {
        if (*(nii_rim_data + i) == 3) {
            tie(x, y, z) = ind2sub_3D(i, size_x, size_y);
            tie(wm_x, wm_y, wm_z) = ind2sub_3D(*(innerGM_id_data + i),
                                               size_x, size_y);
            tie(gm_x, gm_y, gm_z) = ind2sub_3D(*(outerGM_id_data + i),
                                               size_x, size_y);

            // // Normalize distance
            // float dist1 = dist(x, y, z, wm_x, wm_y, wm_z, dX, dY, dZ);
            // float dist2 = dist(x, y, z, gm_x, gm_y, gm_z, dX, dY, dZ);
            // float norm_dist = dist1 / (dist1 + dist2);

            // Normalize distance (completely discrete)
            float dist1 = *(innerGM_dist_data + i);
            float dist2 = *(outerGM_dist_data + i);
            float norm_dist = dist1 / (dist1 + dist2);

            // Difference of normalized distances
            *(normdistdiff_data + i) = (dist1 - dist2) / (dist1 + dist2);

            // Cast distances to integers as number of desired layers
            if (norm_dist != 0) {
                *(nii_layers_data + i) = ceil(nr_layers * norm_dist);
            } else {
                *(nii_layers_data + i) = 1;
            }

            // NOTE: for debugging purposes
            float dist3 = dist(wm_x, wm_y, wm_z, gm_x, gm_y, gm_z, dX, dY, dZ);
            *(err_dist_data + i) = (dist1 + dist2) - dist3;

            // Count inner and outer GM anchor voxels
            j = *(innerGM_id_data + i);
            *(hotspots_data + j) += 1;
            j = *(outerGM_id_data + i);
            *(hotspots_data + j) -= 1;
        }
    }
    save_output_nifti(fin, "layers", nii_layers);
    save_output_nifti(fin, "hotspots", hotspots, false);
    save_output_nifti(fin, "disterror", err_dist, false);
    save_output_nifti(fin, "normdistdiff_diff", normdistdiff, false);

    // ========================================================================
    // Middle gray matter
    // ========================================================================
    cout << "  Start finding middle gray matter..." << endl;
    for (uint32_t i = 0; i != nr_voxels; ++i) {
        if (*(nii_rim_data + i) == 3) {
            // Check sign changes in normalized distance differences between
            // neighbouring voxels on a column path (a.k.a. streamline)
            if (*(normdistdiff_data + i) == 0) {
                *(middleGM_data + i) = 1;
                *(middleGM_id_data + i) = i;
            } else {
                float m = *(normdistdiff_data + i);
                float n;

                // Inner neighbour
                j = *(innerGM_prevstep_id_data + i);
                if (*(nii_rim_data + j) == 3) {
                    n = *(normdistdiff_data + j);
                    if (signbit(m) - signbit(n) != 0) {
                        if (abs(m) < abs(n)) {
                            *(middleGM_data + i) = 1;
                            *(middleGM_id_data + i) = i;
                        } else if (abs(m) > abs(n)) {  // Closer to prev. step
                            *(middleGM_data + j) = 1;
                            *(middleGM_id_data + j) = j;
                        } else {  // Equal +/- normalized distance
                            *(middleGM_data + i) = 1;
                            *(middleGM_id_data + i) = i;
                            *(middleGM_data + j) = 1;
                            *(middleGM_id_data + j) = i;  // On purpose
                        }
                    }
                }

                // Outer neighbour
                j = *(outerGM_prevstep_id_data + i);
                if (*(nii_rim_data + j) == 3) {
                    n = *(normdistdiff_data + j);
                    if (signbit(m) - signbit(n) != 0) {
                        if (abs(m) < abs(n)) {
                            *(middleGM_data + i) = 1;
                            *(middleGM_id_data + i) = i;
                        } else if (abs(m) > abs(n)) {  // Closer to prev. step
                            *(middleGM_data + j) = 1;
                            *(middleGM_id_data + j) = j;
                        } else {  // Equal +/- normalized distance
                            *(middleGM_data + i) = 1;
                            *(middleGM_id_data + i) = i;
                            *(middleGM_data + j) = 1;
                            *(middleGM_id_data + j) = i;  // On purpose
                        }
                    }
                }
            }
        }
    }
    save_output_nifti(fin, "middleGM", middleGM, false);

    // ========================================================================
    // Columns
    // ========================================================================
    cout << "  Start doing columns..." << endl;
    for (uint32_t i = 0; i != nr_voxels; ++i) {
        if (*(nii_rim_data + i) == 3) {
            // Approximate curvature measurement
            j = *(innerGM_id_data + i);
            k = *(outerGM_id_data + i);
            *(curvature_data + i) = *(hotspots_data + j) + *(hotspots_data + k);
            // Re-assign mid-GM id based on curvature
            if (*(curvature_data + i) >= 0) {  // Gyrus
                *(nii_columns_data + i) = j;
            } else {
                *(nii_columns_data + i) = k;
            }

            // TODO(Faruk): Think about re-id midGM as average of id-d voxels.
            // This would be useful for later column downsampling
            // TODO(Faruk): Mean coordinate of columns might be used in
            // equivolume layering
            if (*(middleGM_data + i) == 1) {
                *(middleGM_id_data + i) = *(nii_columns_data + i);
            }
        }
    }

    save_output_nifti(fin, "curvature", curvature, false);
    save_output_nifti(fin, "middleGM_id", middleGM_id, false);
    save_output_nifti(fin, "columns", nii_columns, false);

    cout << "  Finished." << endl;
    return 0;
}
