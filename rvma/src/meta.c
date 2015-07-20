#include "internals.h"

#if 0
int rvma_poll(void)
{
    int rc;
    MPI_Message message;
    MPI_Status  status;

    /* See if a message has been sent. */
    rc = MPI_Mprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, RVMA_GLOBAL_STATE.rvma_comm,
                    &message, &status);
    if (rc!=MPI_SUCCESS) {
        fprintf(stderr,"MPI_Mprobe failed\n");
        return 1;
    }

    int source = status.MPI_SOURCE;
    int tag    = status.MPI_TAG;
    int statrc = status.MPI_ERROR;
    if (statrc!=MPI_SUCCESS) {
        fprintf(stderr,"MPI_Mprobe status error\n");
        return 1;
    }

    /* Determine how many bytes are in this message. */
    int pcount; /* i.e. probe count */
    rc = MPI_Get_elements(&status, MPI_BYTE, &pcount);
    if (rc!=MPI_SUCCESS) {
        fprintf(stderr,"MPI_Get_elements failed\n");
        return 1;
    }

    void * buffer = NULL;
    rc = MPI_Alloc_mem((MPI_Aint)pcount, MPI_INFO_NULL, &buffer);
    if (rc!=MPI_SUCCESS || buffer==NULL) {
        fprintf(stderr,"MPI_Alloc_mem failed\n");
        return 1;
    }

    rc = MPI_Mrecv(buffer, pcount, MPI_BYTE, &message, &status);
    if (rc!=MPI_SUCCESS) {
        fprintf(stderr,"MPI_Mrecv failed\n");
        return 1;
    }

    int rcount; /* i.e. receive count */
    rc = MPI_Get_elements(&status, MPI_BYTE, &rcount);
    if (rc!=MPI_SUCCESS) {
        fprintf(stderr,"MPI_Get_elements failed\n");
        return 1;
    }

    if (rcount!=pcount) {
        fprintf(stderr,"MPI_Mrecv status count less than MPI_Mprobe status count\n");
        return 1;
    }

    /* TODO process message */

    rc = MPI_Free_mem(buffer);
    if (rc!=MPI_SUCCESS) {
        fprintf(stderr,"MPI_Free_mem failed\n");
        return 1;
    }

    return 0;
}
#endif

typedef enum
{
    MSG_INFO_TAG,
    MSG_FLUSH_TAG,
    MSG_GET_INDEX_TAG,
    MSG_PUT_INDEX_TAG,
    MSG_GET_RAW_TAG,
    MSG_PUT_RAW_TAG,
    MSG_LAST_TAG
}
msg_tag_e;

typedef enum
{
    MSG_FLUSH,
    MSG_GET_INDEX,
    MSG_PUT_INDEX,
    MSG_GET_RAW,
    MSG_PUT_RAW,
    MSG_CHT_EXIT,
    MSG_LAST
}
msg_type_e;

typedef struct
{
    msg_type_e   type;
    void *       address;
    int          count;
    MPI_Datatype dt;
}
msg_info_t;

static void * Poll(void * ptr) {

  MPI_Comm comm = *(MPI_Comm*)ptr;

  int rank;
  MPI_Comm_rank(comm, &rank);

  while (1) {

    msg_info_t info;
    MPI_Status status, rstatus;
    MPI_Recv(&info, sizeof(info), MPI_BYTE,
             MPI_ANY_SOURCE, MSG_INFO_TAG, comm,
             &status);
    int source = status.MPI_SOURCE;

    /* poll stuff here */
    switch (info.type)
    {
        case MSG_FLUSH:
            MPI_Send(NULL, 0, MPI_BYTE, source, MSG_FLUSH_TAG, comm);
            break;

        case MSG_GET_RAW:
            MPI_Send(info.address, info.count, info.dt, source, MSG_GET_RAW_TAG, comm);
            break;

        case MSG_PUT_RAW:
            MPI_Recv(info.address, info.count, info.dt, source, MSG_PUT_RAW_TAG, comm, &rstatus);
            int rcount;
            MPI_Get_count(&rstatus, MPI_BYTE, &rcount);
            if (info.count != rcount) {
              printf("CHT PUT message underflow");
              MPI_Abort(comm, info.count-rcount);
            }
            break;

#if 0 /* This does not appear to be the right way to do this. */
        case MSG_GET_INDEX:
            /* lookup data */

            // Read local cells in the range
            int ad;
            void* range; /* This is a template argument... */
            std::vector<int> attribute_ids;
            void* local_cells;
            size_t local_cells_size;
            storage_manager->read_cells(ad, range, attribute_ids, local_cells, local_cells_size);

            MPI_Send(info.address, info.count, info.dt, source, MSG_GET_RAW_TAG, comm);
            break;

        case MSG_PUT_INDEX:
            /* lookup data */
            MPI_Status rstatus;
            MPI_Recv(info.address, info.count, info.dt, source, MSG_PUT_RAW_TAG, comm, &rstatus);
            int rcount;
            MPI_Get_count(&rstatus, MPI_BYTE, &rcount);
            if (info.count != rcount) {
              printf("CHT PUT message underflow");
              MPI_Abort(comm, info.count-rcount);
            }
            break;
#endif

        case MSG_CHT_EXIT:
            if (rank!=source) {
              printf("CHT received EXIT signal from a rank besides self");
              MPI_Abort(comm, source);
            }
            pthread_exit(NULL);
            break;

        default:
            printf("CHT received invalid MSG TAG");
            MPI_Abort(comm, info.type);
            break;
    }
  }
  return NULL;
}

void rvma_thread_start(void) {
  int rc = pthread_create(&(RVMA_GLOBAL_STATE.rvma_thread), NULL, &Poll, &(RVMA_GLOBAL_STATE.rvma_comm));
  if(rc != 0) {
    printf("pthread_create failed");
    MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
  }
}

/** Tells the comm thread to exit. */
void rvma_thread_stop(void) {
  int rank;
  MPI_Comm_rank((RVMA_GLOBAL_STATE.rvma_comm), &rank);

  msg_info_t info;
  info.type = MSG_CHT_EXIT;

  /* Synchronous send will wait until the receive is matched,
   * so we know that the thread has received the exit command. */
  MPI_Ssend(&info, sizeof(msg_info_t), MPI_BYTE, rank, MSG_INFO_TAG, (RVMA_GLOBAL_STATE.rvma_comm));
  void * rv; /* unused; thread returns NULL */
  int rc = pthread_join((RVMA_GLOBAL_STATE.rvma_thread), &rv);
  if (rc!=0) {
    printf("pthread_join failed");
    MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
  }
}

void rvma_am_flush(int remote_proc)
{
    /* Verify that C99 struct initialization is fully compliant with ISO C++... */
    msg_info_t info = { .type    = MSG_FLUSH,
                        .address = NULL,
                        .count   = 0,
                        .dt      = MPI_BYTE };
    MPI_Send(&info, sizeof(msg_info_t), MPI_BYTE, remote_proc, MSG_INFO_TAG, (RVMA_GLOBAL_STATE.rvma_comm));
    MPI_Recv(NULL, info.count, info.dt, remote_proc, MSG_FLUSH_TAG, (RVMA_GLOBAL_STATE.rvma_comm), MPI_STATUS_IGNORE);
}

void rvma_am_get_raw(void* output, void * remote_input, int size, int remote_proc)
{
    /* Verify that C99 struct initialization is fully compliant with ISO C++... */
    msg_info_t info = { .type    = MSG_GET_RAW,
                        .address = remote_input,
                        .count   = size,
                        .dt      = MPI_BYTE };
    MPI_Send(&info, sizeof(msg_info_t), MPI_BYTE, remote_proc, MSG_INFO_TAG, (RVMA_GLOBAL_STATE.rvma_comm));

    MPI_Status status;
    MPI_Recv(output, info.count, info.dt, remote_proc, MSG_GET_RAW_TAG, (RVMA_GLOBAL_STATE.rvma_comm), &status);

    int rcount;
    MPI_Get_count(&status, MPI_BYTE, &rcount);
    if (info.count != rcount) {
      printf("Get_raw message underflow");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), info.count-rcount);
    }
}

void rvma_am_put_raw(void* input, void * remote_output, int size, int remote_proc)
{
    /* Verify that C99 struct initialization is fully compliant with ISO C++... */
    msg_info_t info = { .type    = MSG_PUT_RAW,
                        .address = remote_output,
                        .count   = size,
                        .dt      = MPI_BYTE };
    MPI_Send(&info, sizeof(msg_info_t), MPI_BYTE, remote_proc, MSG_INFO_TAG, (RVMA_GLOBAL_STATE.rvma_comm));
    MPI_Send(input, info.count, info.dt, remote_proc, MSG_PUT_RAW_TAG, (RVMA_GLOBAL_STATE.rvma_comm));
}

void rvma_am_put_index(void* input, void * remote_output, int size, int remote_proc)
{
    /* TODO pack metadata for lookup... */
    msg_info_t info = { .type    = MSG_PUT_INDEX,
                        .address = remote_output,
                        .count   = size,
                        .dt      = MPI_BYTE };
    MPI_Send(&info, sizeof(msg_info_t), MPI_BYTE, remote_proc, MSG_INFO_TAG, (RVMA_GLOBAL_STATE.rvma_comm));
    MPI_Send(input, info.count, info.dt, remote_proc, MSG_PUT_INDEX_TAG, (RVMA_GLOBAL_STATE.rvma_comm));
}

void rvma_am_get_index(void* output, void * remote_input, int size, int remote_proc)
{
    /* TODO pack metadata for lookup... */
    msg_info_t info = { .type    = MSG_GET_INDEX,
                        .address = remote_input,
                        .count   = size,
                        .dt      = MPI_BYTE };
    MPI_Send(&info, sizeof(msg_info_t), MPI_BYTE, remote_proc, MSG_INFO_TAG, (RVMA_GLOBAL_STATE.rvma_comm));

    MPI_Status status;
    MPI_Recv(output, info.count, info.dt, remote_proc, MSG_GET_INDEX_TAG, (RVMA_GLOBAL_STATE.rvma_comm), &status);

    int rcount;
    MPI_Get_count(&status, MPI_BYTE, &rcount);
    if (info.count != rcount) {
      printf("Get_index message underflow");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), info.count-rcount);
    }
}

void rvma_old_init(MPI_Comm user_comm, int* argc, char*** argv) {

  int is_init;
  {
    int rc = MPI_Initialized(&is_init);
    if(rc != MPI_SUCCESS) {
      printf("MPI_Initialized failed");
      MPI_Abort(user_comm, rc);
    }
  }

  int thread_requested = MPI_THREAD_MULTIPLE;
  int thread_provided;
  if (is_init) {
    int rc = MPI_Query_thread(&thread_provided);
    if(rc != MPI_SUCCESS) {
      printf("MPI_Init_thread failed");
      MPI_Abort(user_comm, rc);
    }
    if(thread_provided<thread_requested) {
      printf("MPI: insufficient thread level");
      MPI_Abort(user_comm, thread_provided);
    }
    RVMA_GLOBAL_STATE.rvma_owns_mpi = 0;
  } else {
    int rc = MPI_Init_thread(argc, argv, thread_requested,
                             &thread_provided);
    if(rc != MPI_SUCCESS) {
      printf("MPI_Init_thread failed");
      MPI_Abort(user_comm, rc);
    }
    if(thread_provided<thread_requested) {
      printf("MPI: insufficient thread level");
      MPI_Abort(user_comm, thread_provided);
    }
    RVMA_GLOBAL_STATE.rvma_owns_mpi = 1;
  }

  /* User cannot provide valid comm if MPI is not
   * initialized.  Ignore silly case of MPI_Comm_SELF. */
  if (!is_init && user_comm!=MPI_COMM_WORLD)
  {
    printf("Initialize MPI first!");
    MPI_Abort(user_comm, 1);
  }

  {
    /* Query for intercommunicator (unlikely) */
    int is_intercomm;
    int rc = MPI_Comm_test_inter(user_comm, &is_intercomm);
    if (rc!=MPI_SUCCESS) {
      printf("MPI_Comm_test_inter failed");
      MPI_Abort(user_comm, 1);
    }
    if (is_intercomm) {
      printf("Intercommunicators not okay");
      MPI_Abort(user_comm, 1);
    }
  }

  /* Duplicate the users communicator to avoid any cross-talk. */
  {
    int rc = MPI_Comm_dup(user_comm, &(RVMA_GLOBAL_STATE.rvma_comm));
    if(rc != MPI_SUCCESS) {
      printf("MPI_Comm_dup failed");
      MPI_Abort(user_comm, rc);
    }
  }

  {
    /* Create the global window to be used for all comms */
    MPI_Info win_info = MPI_INFO_NULL;
    int rc = MPI_Info_create(&win_info);
    if(rc != MPI_SUCCESS) {
      printf("MPI_Info_create failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }

    /* Do not order atomic put */
    rc = MPI_Info_set(win_info,"accumulate_ordering","");
    if(rc != MPI_SUCCESS) {
      printf("MPI_Info_set failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }
    /* Assume only REPLACE and NO_OP i.e. atomic put */
    rc = MPI_Info_set(win_info,"accumulate_ops","same_op_no_op");
    if(rc != MPI_SUCCESS) {
      printf("MPI_Info_set failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }
    rc = MPI_Win_create_dynamic(win_info, (RVMA_GLOBAL_STATE.rvma_comm), &(RVMA_GLOBAL_STATE.rvma_win) );
    if(rc != MPI_SUCCESS) {
      printf("MPI_Win_create_dynamic failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }
    rc = MPI_Info_free(&win_info);
    if(rc != MPI_SUCCESS) {
      printf("MPI_Info_free failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }

    /* Enter "PGAS mode" */
    rc = MPI_Win_lock_all(MPI_MODE_NOCHECK, (RVMA_GLOBAL_STATE.rvma_win));
    if(rc != MPI_SUCCESS) {
      printf("MPI_Win_lock_all failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }
  }

  rvma_thread_start();

  return;
}

void rvma_old_finalize(void) {

  rvma_thread_stop();

  {
    /* Exit "PGAS mode" */
    int rc = MPI_Win_unlock_all((RVMA_GLOBAL_STATE.rvma_win));
    if(rc != MPI_SUCCESS) {
      printf("MPI_Win_unlock_all failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }
  }

  {
    int rc = MPI_Win_free(&(RVMA_GLOBAL_STATE.rvma_win) );
    if(rc != MPI_SUCCESS) {
      printf("MPI_Win_free failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }
  }

  {
    int rc = MPI_Comm_free(&(RVMA_GLOBAL_STATE.rvma_comm));
    if(rc != MPI_SUCCESS) {
      printf("MPI_Comm_free failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }
  }

  if (RVMA_GLOBAL_STATE.rvma_owns_mpi) {
    int rc = MPI_Finalize();
    if(rc != MPI_SUCCESS) {
      printf("MPI_Finalize failed");
      MPI_Abort((RVMA_GLOBAL_STATE.rvma_comm), rc);
    }
  }
}

