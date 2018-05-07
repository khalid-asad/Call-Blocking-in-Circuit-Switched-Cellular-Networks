
/*
 * 
 * Call Blocking in Circuit Switched Networks
 * 
 * Copyright (C) 2014 Terence D. Todd
 * Hamilton, Ontario, CANADA
 * todd@mcmaster.ca
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 */

/*******************************************************************************/

#include <stdio.h>

#include "trace.h"
#include "main.h"
#include "output.h"
#include "simparameters.h"
#include "call_departure.h"

/*******************************************************************************/

/* 
 * Function to schedule an end of call event. 
 */

long int
schedule_end_call_on_channel_event(Simulation_Run_Ptr simulation_run,
				   double event_time,
				   void * channel) 
{
  Event new_event;

  new_event.description = "End of Call";
  new_event.function = end_call_on_channel_event;
  new_event.attachment = channel;

  return simulation_run_schedule_event(simulation_run, new_event, event_time);
}

/*******************************************************************************/

/* 
 * Function which handles end of call events.
 */

void
end_call_on_channel_event(Simulation_Run_Ptr simulation_run, void * c_ptr)
{
  Call_Ptr this_call, next_call;
  Channel_Ptr channel;
  Simulation_Run_Data_Ptr sim_data;
  double now;
  double call_time = 0.0;
  int q_count = 0;

  channel = (Channel_Ptr) c_ptr;

  now = simulation_run_get_time(simulation_run);
  sim_data = simulation_run_data(simulation_run);

  this_call = (Call_Ptr) server_get(channel);

  TRACE(printf("End Of Call.\n"););

  /* Collect statistics. */
  sim_data->number_of_calls_processed++;
  sim_data->accumulated_call_time += now - this_call->arrive_time;

  output_progress_msg_to_screen(simulation_run);

  /* This call is done. */
  xfree((void*) this_call);

  /*
  * See if there are calls waiting in the buffer. If so, take the next one immediately.
  */
  if ((channel = get_free_channel(simulation_run)) != NULL) {
	  if (fifoqueue_size(sim_data->buffer) > 0) {
		  for (q_count = 0; q_count < fifoqueue_size(sim_data->buffer); q_count++) {
			  next_call = (Call_Ptr)fifoqueue_get(sim_data->buffer);
			  call_time = simulation_run_get_time(simulation_run) - this_call->arrive_time;
			  sim_data->queue_wait_time += call_time;
			  if (call_time >= MAX_WAIT_TIME) {
				  sim_data->blocked_call_count++;
				  TRACE(printf("Call Blocked"););
			  }
			  else {
				  next_call->arrive_time = simulation_run_get_time(simulation_run);
				  next_call->call_duration = get_call_duration();
				  server_put(channel, (void*)next_call);
				  next_call->channel = channel;
				  schedule_end_call_on_channel_event(simulation_run, simulation_run_get_time(simulation_run) + get_call_duration(), (void *)channel);
				  TRACE(printf("Waiting Call Completed"););
				  break;
			  }
		  }
	  }
  }
}


