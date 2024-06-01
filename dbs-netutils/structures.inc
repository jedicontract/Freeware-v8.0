C		    Structures to define VMS data types

C	These are based on information in Appendix A of the manual
C	"Introduction to VMS System Routines".  Please consult this
C	manual for further information.

C	27-Sep-1990, DBS; Version X1-001
C 001 -	Original version.  (Taken from something similar but some changes
C	have been made and are as yet untested.)

	structure /address_range/
		integer*4	low_address
		integer*4	high_address
	end structure !address_range
	volatile /address_range/

	structure /io_status_block/
		integer*2	value
		integer*2	count
		integer*4	device_info
	end structure !io_status_block
	volatile /io_status_block/

	structure /item_list_2/
	  union
	    map
	       	integer*2	buflen
		integer*2	code
		integer*4	bufadr
	    end map
	    map
	       	integer*2	buffer_length
		integer*2	item_code
		integer*4	buffer_address
	    end map
	    map
		integer*4	end_list /0/
	    end map
	  end union
	end structure !item_list_2
	volatile /item_list_2/

	structure /item_list_3/
	  union
	    map
	       	integer*2	buflen
		integer*2	code
		integer*4	bufadr
		integer*4	retlenadr
	    end map
	    map
	       	integer*2	buffer_length
		integer*2	item_code
		integer*4	buffer_address
		integer*4	return_length_address
	    end map
	    map
		integer*4	end_list /0/
	    end map
	  end union
	end structure !item_list_3
	volatile /item_list_3/

	structure /item_list_pair/
	  union
	    map
		integer*4	code 
		integer*4	value
	    end map
	    map
		integer*4	item_code 
		integer*4	item_value
	    end map
	    map
		integer*4	end_list /0/
	    end map
	  end union
	end structure !item_list_pair
	volatile /item_list_pair/

	structure /item_quota_list/
	  union
	    map
		byte		quota_name
		integer*4	quota_value
	    end map
	    map
		byte		end_list /0/
	    end map
	  end union
	end structure !item_quota_list
	volatile /item_quota_list/

	structure /lock_status_block/
		integer*2	cond_value
		integer*2	unused
		integer*4	lock_id
		byte		lock_value(16)
	end structure !lock_status_block
	volatile /lock_status_block/

	structure /rights_holder/
		integer*4	rights_id
		integer*4	rights_mask
	end structure !rights_holder
	volatile /rights_holder/

	structure /uic/
	  union
	    map
		integer*4	uic
	    end map
	    map
		integer*2	member
		integer*2	group
	    end map
	  end union
	end structure !uic
	volatile /uic/
