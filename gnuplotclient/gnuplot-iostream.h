

#ifndef GNUPLOT_IOSTREAM_H
#define GNUPLOT_IOSTREAM_H


#define GNUPLOT_IOSTREAM_VERSION 2

#ifndef GNUPLOT_ENABLE_CXX11
#	define GNUPLOT_ENABLE_CXX11 (__cplusplus >= 201103)
#endif

#include <cstdio>
#ifdef GNUPLOT_ENABLE_PTY
#	include <termios.h>
#	include <unistd.h>
#	include <pty.h>
#endif // GNUPLOT_ENABLE_PTY

// C++ system includes
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <iomanip>
#include <vector>
#include <complex>
#include <cstdlib>
#include <cmath>

#if GNUPLOT_ENABLE_CXX11
#	include <tuple>
#endif

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/version.hpp>
#include <boost/utility.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/mpl/bool.hpp>
#if BOOST_VERSION >= 104600
#	define GNUPLOT_USE_TMPFILE
#	include <boost/filesystem.hpp>
#endif

#include <boost/cstdint.hpp>

#ifdef GNUPLOT_ENABLE_BLITZ
#	include <blitz/array.h>
#endif

#ifdef BOOST_STATIC_ASSERT_MSG
#	define GNUPLOT_STATIC_ASSERT_MSG(cond, msg) BOOST_STATIC_ASSERT_MSG((cond), msg)
#else
#	define GNUPLOT_STATIC_ASSERT_MSG(cond, msg) BOOST_STATIC_ASSERT((cond))
#endif

#ifdef GNUPLOT_DEPRECATE_WARN
#	ifdef __GNUC__
#		define GNUPLOT_DEPRECATE(msg) __attribute__ ((deprecated(msg)))
#	elif defined(_MSC_VER)
#		define GNUPLOT_DEPRECATE(msg) __declspec(deprecated(msg))
#	else
#		define GNUPLOT_DEPRECATE(msg)
#	endif
#else
#	define GNUPLOT_DEPRECATE(msg)
#endif

#ifdef _WIN32
#	include <windows.h>
#	define GNUPLOT_PCLOSE _pclose
#	define GNUPLOT_POPEN  _popen
#	define GNUPLOT_FILENO _fileno
#else
#	define GNUPLOT_PCLOSE pclose
#	define GNUPLOT_POPEN  popen
#	define GNUPLOT_FILENO fileno
#endif

#ifdef _WIN32
#	define GNUPLOT_ISNAN _isnan
#else
#	define GNUPLOT_ISNAN std::isnan
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1400
#	define GNUPLOT_MSVC_WARNING_4996_PUSH \
		__pragma(warning(push)) \
		__pragma(warning(disable:4996))
#	define GNUPLOT_MSVC_WARNING_4996_POP \
		__pragma(warning(pop))
#else
#	define GNUPLOT_MSVC_WARNING_4996_PUSH
#	define GNUPLOT_MSVC_WARNING_4996_POP
#endif

#ifndef GNUPLOT_DEFAULT_COMMAND
#ifdef _WIN32
#	define GNUPLOT_DEFAULT_COMMAND "gnuplot -persist 2> NUL"
#else
#	define GNUPLOT_DEFAULT_COMMAND "gnuplot -persist"
#endif
#endif


namespace gnuplotio {

template <typename T>
struct dont_treat_as_stl_container {
	typedef boost::mpl::bool_<false> type;
};

BOOST_MPL_HAS_XXX_TRAIT_DEF(value_type)
BOOST_MPL_HAS_XXX_TRAIT_DEF(const_iterator)

template <typename T>
struct is_like_stl_container {
	typedef boost::mpl::and_<
			typename has_value_type<T>::type,
			typename has_const_iterator<T>::type,
			boost::mpl::not_<dont_treat_as_stl_container<T> >
		> type;
	static const bool value = type::value;
};

template <typename T>
struct is_boost_tuple_nulltype {
	static const bool value = false;
	typedef boost::mpl::bool_<value> type;
};

template <>
struct is_boost_tuple_nulltype<boost::tuples::null_type> {
	static const bool value = true;
	typedef boost::mpl::bool_<value> type;
};

BOOST_MPL_HAS_XXX_TRAIT_DEF(head_type)
BOOST_MPL_HAS_XXX_TRAIT_DEF(tail_type)

template <typename T>
struct is_boost_tuple {
	typedef boost::mpl::and_<
			typename has_head_type<T>::type,
			typename has_tail_type<T>::type
		> type;
	static const bool value = type::value;
};

#ifdef GNUPLOT_USE_TMPFILE
class GnuplotTmpfile {
public:
	GnuplotTmpfile() :
		file(boost::filesystem::unique_path(
			boost::filesystem::temp_directory_path() /
			"tmp-gnuplot-%%%%-%%%%-%%%%-%%%%"))
	{ }

private:
	// noncopyable
	GnuplotTmpfile(const GnuplotTmpfile &);
	const GnuplotTmpfile& operator=(const GnuplotTmpfile &);

public:
	~GnuplotTmpfile() {
		// it is never good to throw exceptions from a destructor
		try {
			remove(file);
		} catch(const std::exception &) {
			std::cerr << "Failed to remove temporary file " << file << std::endl;
		}
	}

public:
	boost::filesystem::path file;
};
#endif // GNUPLOT_USE_TMPFILE

class GnuplotFeedback {
public:
	GnuplotFeedback() { }
	virtual ~GnuplotFeedback() { }
	virtual std::string filename() const = 0;
	virtual FILE *handle() const = 0;

private:
	// noncopyable
	GnuplotFeedback(const GnuplotFeedback &);
	const GnuplotFeedback& operator=(const GnuplotFeedback &);
};

#ifdef GNUPLOT_ENABLE_PTY
#define GNUPLOT_ENABLE_FEEDBACK
class GnuplotFeedbackPty : public GnuplotFeedback {
public:
	explicit GnuplotFeedbackPty(bool debug_messages) :
		pty_fn(),
		pty_fh(NULL),
		master_fd(-1),
		slave_fd(-1)
	{
	// adapted from http://www.gnuplot.info/files/gpReadMouseTest.c
		if(0 > openpty(&master_fd, &slave_fd, NULL, NULL, NULL)) {
			perror("openpty");
			throw std::runtime_error("openpty failed");
		}
		char pty_fn_buf[1024];
		if(ttyname_r(slave_fd, pty_fn_buf, 1024)) {
			perror("ttyname_r");
			throw std::runtime_error("ttyname failed");
		}
		pty_fn = std::string(pty_fn_buf);
		if(debug_messages) {
			std::cerr << "feedback_fn=" << pty_fn << std::endl;
		}

		// disable echo
		struct termios tios;
		if(tcgetattr(slave_fd, &tios) < 0) {
			perror("tcgetattr");
			throw std::runtime_error("tcgetattr failed");
		}
		tios.c_lflag &= ~(ECHO | ECHONL);
		if(tcsetattr(slave_fd, TCSAFLUSH, &tios) < 0) {
			perror("tcsetattr");
			throw std::runtime_error("tcsetattr failed");
		}

		pty_fh = fdopen(master_fd, "r");
		if(!pty_fh) {
			throw std::runtime_error("fdopen failed");
		}
	}

private:
	// noncopyable
	GnuplotFeedbackPty(const GnuplotFeedbackPty &);
	const GnuplotFeedbackPty& operator=(const GnuplotFeedbackPty &);

public:
	~GnuplotFeedbackPty() {
		if(pty_fh) fclose(pty_fh);
		if(master_fd > 0) ::close(master_fd);
		if(slave_fd  > 0) ::close(slave_fd);
	}

	std::string filename() const {
		return pty_fn;
	}

	FILE *handle() const {
		return pty_fh;
	}

private:
	std::string pty_fn;
	FILE *pty_fh;
	int master_fd, slave_fd;
};
#endif // GNUPLOT_ENABLE_PTY, GNUPLOT_USE_TMPFILE
template <typename T, typename Enable=void>
struct TextSender {
	static void send(std::ostream &stream, const T &v) {
		stream << v;
	}
};

template <typename T, typename Enable=void>
struct BinarySender {
	GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "BinarySender class not specialized for this type");

	static void send(std::ostream &stream, const T &v);
};

template <typename T>
struct FlatBinarySender {
	static void send(std::ostream &stream, const T &v) {
		stream.write(reinterpret_cast<const char *>(&v), sizeof(T));
	}
};

template <typename T, typename Enable=void>
struct BinfmtSender {
	GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "BinfmtSender class not specialized for this type");

	// This is here to avoid further compilation errors, beyond what the assert prints.
	static void send(std::ostream &);
};

template<> struct BinfmtSender< float> { static void send(std::ostream &stream) { stream << "%float";  } };
template<> struct BinfmtSender<double> { static void send(std::ostream &stream) { stream << "%double"; } };
template<> struct BinfmtSender<boost::  int8_t> { static void send(std::ostream &stream) { stream << "%int8";   } };
template<> struct BinfmtSender<boost:: uint8_t> { static void send(std::ostream &stream) { stream << "%uint8";  } };
template<> struct BinfmtSender<boost:: int16_t> { static void send(std::ostream &stream) { stream << "%int16";  } };
template<> struct BinfmtSender<boost::uint16_t> { static void send(std::ostream &stream) { stream << "%uint16"; } };
template<> struct BinfmtSender<boost:: int32_t> { static void send(std::ostream &stream) { stream << "%int32";  } };
template<> struct BinfmtSender<boost::uint32_t> { static void send(std::ostream &stream) { stream << "%uint32"; } };
template<> struct BinfmtSender<boost:: int64_t> { static void send(std::ostream &stream) { stream << "%int64";  } };
template<> struct BinfmtSender<boost::uint64_t> { static void send(std::ostream &stream) { stream << "%uint64"; } };

template<> struct BinarySender< float> : public FlatBinarySender< float> { };
template<> struct BinarySender<double> : public FlatBinarySender<double> { };
template<> struct BinarySender<boost::  int8_t> : public FlatBinarySender<boost::  int8_t> { };
template<> struct BinarySender<boost:: uint8_t> : public FlatBinarySender<boost:: uint8_t> { };
template<> struct BinarySender<boost:: int16_t> : public FlatBinarySender<boost:: int16_t> { };
template<> struct BinarySender<boost::uint16_t> : public FlatBinarySender<boost::uint16_t> { };
template<> struct BinarySender<boost:: int32_t> : public FlatBinarySender<boost:: int32_t> { };
template<> struct BinarySender<boost::uint32_t> : public FlatBinarySender<boost::uint32_t> { };
template<> struct BinarySender<boost:: int64_t> : public FlatBinarySender<boost:: int64_t> { };
template<> struct BinarySender<boost::uint64_t> : public FlatBinarySender<boost::uint64_t> { };

template <typename T>
struct CastIntTextSender {
	static void send(std::ostream &stream, const T &v) {
		stream << int(v);
	}
};
template<> struct TextSender<          char> : public CastIntTextSender<          char> { };
template<> struct TextSender<   signed char> : public CastIntTextSender<   signed char> { };
template<> struct TextSender< unsigned char> : public CastIntTextSender< unsigned char> { };

// Make sure that the same not-a-number string is printed on all platforms.
template <typename T>
struct FloatTextSender {
	static void send(std::ostream &stream, const T &v) {
		if(GNUPLOT_ISNAN(v)) { stream << "nan"; } else { stream << v; }
	}
};
template<> struct TextSender<      float> : FloatTextSender<      float> { };
template<> struct TextSender<     double> : FloatTextSender<     double> { };
template<> struct TextSender<long double> : FloatTextSender<long double> { };


template <typename T, typename U>
struct TextSender<std::pair<T, U> > {
	static void send(std::ostream &stream, const std::pair<T, U> &v) {
		TextSender<T>::send(stream, v.first);
		stream << " ";
		TextSender<U>::send(stream, v.second);
	}
};

template <typename T, typename U>
struct BinfmtSender<std::pair<T, U> > {
	static void send(std::ostream &stream) {
		BinfmtSender<T>::send(stream);
		BinfmtSender<U>::send(stream);
	}
};

template <typename T, typename U>
struct BinarySender<std::pair<T, U> > {
	static void send(std::ostream &stream, const std::pair<T, U> &v) {
		BinarySender<T>::send(stream, v.first);
		BinarySender<U>::send(stream, v.second);
	}
};


template <typename T>
struct TextSender<std::complex<T> > {
	static void send(std::ostream &stream, const std::complex<T> &v) {
		TextSender<T>::send(stream, v.real());
		stream << " ";
		TextSender<T>::send(stream, v.imag());
	}
};

template <typename T>
struct BinfmtSender<std::complex<T> > {
	static void send(std::ostream &stream) {
		BinfmtSender<T>::send(stream);
		BinfmtSender<T>::send(stream);
	}
};

template <typename T>
struct BinarySender<std::complex<T> > {
	static void send(std::ostream &stream, const std::complex<T> &v) {
		BinarySender<T>::send(stream, v.real());
		BinarySender<T>::send(stream, v.imag());
	}
};


template <typename T>
struct TextSender<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			boost::mpl::not_<is_boost_tuple_nulltype<typename T::tail_type> >
		>
	>::type
> {
	static void send(std::ostream &stream, const T &v) {
		TextSender<typename T::head_type>::send(stream, v.get_head());
		stream << " ";
		TextSender<typename T::tail_type>::send(stream, v.get_tail());
	}
};

template <typename T>
struct TextSender<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			is_boost_tuple_nulltype<typename T::tail_type>
		>
	>::type
> {
	static void send(std::ostream &stream, const T &v) {
		TextSender<typename T::head_type>::send(stream, v.get_head());
	}
};

template <typename T>
struct BinfmtSender<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			boost::mpl::not_<is_boost_tuple_nulltype<typename T::tail_type> >
		>
	>::type
> {
	static void send(std::ostream &stream) {
		BinfmtSender<typename T::head_type>::send(stream);
		stream << " ";
		BinfmtSender<typename T::tail_type>::send(stream);
	}
};

template <typename T>
struct BinfmtSender<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			is_boost_tuple_nulltype<typename T::tail_type>
		>
	>::type
> {
	static void send(std::ostream &stream) {
		BinfmtSender<typename T::head_type>::send(stream);
	}
};

template <typename T>
struct BinarySender<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			boost::mpl::not_<is_boost_tuple_nulltype<typename T::tail_type> >
		>
	>::type
> {
	static void send(std::ostream &stream, const T &v) {
		BinarySender<typename T::head_type>::send(stream, v.get_head());
		BinarySender<typename T::tail_type>::send(stream, v.get_tail());
	}
};

template <typename T>
struct BinarySender<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			is_boost_tuple_nulltype<typename T::tail_type>
		>
	>::type
> {
	static void send(std::ostream &stream, const T &v) {
		BinarySender<typename T::head_type>::send(stream, v.get_head());
	}
};


#if GNUPLOT_ENABLE_CXX11

// http://stackoverflow.com/questions/6245735/pretty-print-stdtuple

template<std::size_t> struct int_{}; // compile-time counter

template <typename Tuple, std::size_t I>
void std_tuple_formatcode_helper(std::ostream &stream, const Tuple *, int_<I>) {
	std_tuple_formatcode_helper(stream, (const Tuple *)(0), int_<I-1>());
	stream << " ";
	BinfmtSender<typename std::tuple_element<I, Tuple>::type>::send(stream);
}

template <typename Tuple>
void std_tuple_formatcode_helper(std::ostream &stream, const Tuple *, int_<0>) {
	BinfmtSender<typename std::tuple_element<0, Tuple>::type>::send(stream);
}

template <typename... Args>
struct BinfmtSender<std::tuple<Args...> > {
	typedef typename std::tuple<Args...> Tuple;

	static void send(std::ostream &stream) {
		std_tuple_formatcode_helper(stream, (const Tuple *)(0), int_<sizeof...(Args)-1>());
	}
};

template <typename Tuple, std::size_t I>
void std_tuple_textsend_helper(std::ostream &stream, const Tuple &v, int_<I>) {
	std_tuple_textsend_helper(stream, v, int_<I-1>());
	stream << " ";
	TextSender<typename std::tuple_element<I, Tuple>::type>::send(stream, std::get<I>(v));
}

template <typename Tuple>
void std_tuple_textsend_helper(std::ostream &stream, const Tuple &v, int_<0>) {
	TextSender<typename std::tuple_element<0, Tuple>::type>::send(stream, std::get<0>(v));
}

template <typename... Args>
struct TextSender<std::tuple<Args...> > {
	typedef typename std::tuple<Args...> Tuple;

	static void send(std::ostream &stream, const Tuple &v) {
		std_tuple_textsend_helper(stream, v, int_<sizeof...(Args)-1>());
	}
};

template <typename Tuple, std::size_t I>
void std_tuple_binsend_helper(std::ostream &stream, const Tuple &v, int_<I>) {
	std_tuple_binsend_helper(stream, v, int_<I-1>());
	BinarySender<typename std::tuple_element<I, Tuple>::type>::send(stream, std::get<I>(v));
}

template <typename Tuple>
void std_tuple_binsend_helper(std::ostream &stream, const Tuple &v, int_<0>) {
	BinarySender<typename std::tuple_element<0, Tuple>::type>::send(stream, std::get<0>(v));
}

template <typename... Args>
struct BinarySender<std::tuple<Args...> > {
	typedef typename std::tuple<Args...> Tuple;

	static void send(std::ostream &stream, const Tuple &v) {
		std_tuple_binsend_helper(stream, v, int_<sizeof...(Args)-1>());
	}
};

#endif // GNUPLOT_ENABLE_CXX11

struct Error_WasNotContainer {
	typedef void subiter_type;
};

struct Error_InappropriateDeref { };

// The unspecialized version of this class gives traits for things that are *not* arrays.
template <typename T, typename Enable=void>
class ArrayTraits {
public:
	typedef Error_WasNotContainer value_type;
	typedef Error_WasNotContainer range_type;
	static const bool is_container = false;
	static const bool allow_auto_unwrap = false;
	static const size_t depth = 0;

	static range_type get_range(const T &) {
		GNUPLOT_STATIC_ASSERT_MSG((sizeof(T)==0), "argument was not a container");
		throw std::logic_error("static assert should have been triggered by this point");
	}
};

template <typename V>
class ArrayTraitsDefaults {
public:
	typedef V value_type;

	static const bool is_container = true;
	static const bool allow_auto_unwrap = true;
	static const size_t depth = ArrayTraits<V>::depth + 1;
};

template <typename T>
class ArrayTraits<T&> : public ArrayTraits<T> { };

#if GNUPLOT_ENABLE_CXX11
template <typename T>
class ArrayTraits<T&&> : public ArrayTraits<T> { };
#endif


template <typename TI, typename TV>
class IteratorRange {
public:
	IteratorRange() { }
	IteratorRange(const TI &_it, const TI &_end) : it(_it), end(_end) { }

	static const bool is_container = ArrayTraits<TV>::is_container;
	typedef typename boost::mpl::if_c<is_container,
			Error_InappropriateDeref, TV>::type value_type;
	typedef typename ArrayTraits<TV>::range_type subiter_type;

	bool is_end() const { return it == end; }

	void inc() { ++it; }

	value_type deref() const {
		GNUPLOT_STATIC_ASSERT_MSG(sizeof(TV) && !is_container,
			"deref called on nested container");
		return *it;
	}

	subiter_type deref_subiter() const {
		GNUPLOT_STATIC_ASSERT_MSG(sizeof(TV) && is_container,
			"deref_iter called on non-nested container");
		return ArrayTraits<TV>::get_range(*it);
	}

private:
	TI it, end;
};

template <typename T>
class ArrayTraits<T,
	typename boost::enable_if<is_like_stl_container<T> >::type
> : public ArrayTraitsDefaults<typename T::value_type> {
public:
	typedef IteratorRange<typename T::const_iterator, typename T::value_type> range_type;

	static range_type get_range(const T &arg) {
		return range_type(arg.begin(), arg.end());
	}
};


template <typename T, size_t N>
class ArrayTraits<T[N]> : public ArrayTraitsDefaults<T> {
public:
	typedef IteratorRange<const T*, T> range_type;

	static range_type get_range(const T (&arg)[N]) {
		return range_type(arg, arg+N);
	}
};


template <typename RT, typename RU>
class PairOfRange {
	template <typename T, typename U, typename PrintMode>
	friend void deref_and_print(std::ostream &, const PairOfRange<T, U> &, PrintMode);

public:
	PairOfRange() { }
	PairOfRange(const RT &_l, const RU &_r) : l(_l), r(_r) { }

	static const bool is_container = RT::is_container && RU::is_container;

	typedef std::pair<typename RT::value_type, typename RU::value_type> value_type;
	typedef PairOfRange<typename RT::subiter_type, typename RU::subiter_type> subiter_type;

	bool is_end() const {
		bool el = l.is_end();
		bool er = r.is_end();
		if(el != er) {
			throw std::length_error("columns were different lengths");
		}
		return el;
	}

	void inc() {
		l.inc();
		r.inc();
	}

	value_type deref() const {
		return std::make_pair(l.deref(), r.deref());
	}

	subiter_type deref_subiter() const {
		return subiter_type(l.deref_subiter(), r.deref_subiter());
	}

private:
	RT l;
	RU r;
};

template <typename T, typename U>
class ArrayTraits<std::pair<T, U> > {
public:
	typedef PairOfRange<typename ArrayTraits<T>::range_type, typename ArrayTraits<U>::range_type> range_type;
	typedef std::pair<typename ArrayTraits<T>::value_type, typename ArrayTraits<U>::value_type> value_type;
	static const bool is_container = ArrayTraits<T>::is_container && ArrayTraits<U>::is_container;
	// Don't allow colwrap since it's already wrapped.
	static const bool allow_auto_unwrap = false;
	// It is allowed for l_depth != r_depth, for example one column could be 'double' and the
	// other column could be 'vector<double>'.
	static const size_t l_depth = ArrayTraits<T>::depth;
	static const size_t r_depth = ArrayTraits<U>::depth;
	static const size_t depth = (l_depth < r_depth) ? l_depth : r_depth;

	static range_type get_range(const std::pair<T, U> &arg) {
		return range_type(
			ArrayTraits<T>::get_range(arg.first),
			ArrayTraits<U>::get_range(arg.second)
		);
	}
};


template <typename T>
class ArrayTraits<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			boost::mpl::not_<is_boost_tuple_nulltype<typename T::tail_type> >
		>
	>::type
> : public ArrayTraits<
	typename std::pair<
		typename T::head_type,
		typename T::tail_type
	>
> {
public:
	typedef typename T::head_type HT;
	typedef typename T::tail_type TT;

	typedef ArrayTraits<typename std::pair<HT, TT> > parent;

	static typename parent::range_type get_range(const T &arg) {
		return typename parent::range_type(
			ArrayTraits<HT>::get_range(arg.get_head()),
			ArrayTraits<TT>::get_range(arg.get_tail())
		);
	}
};

template <typename T>
class ArrayTraits<T,
	typename boost::enable_if<
		boost::mpl::and_<
			is_boost_tuple<T>,
			is_boost_tuple_nulltype<typename T::tail_type>
		>
	>::type
> : public ArrayTraits<
	typename T::head_type
> {
	typedef typename T::head_type HT;

	typedef ArrayTraits<HT> parent;

public:
	static typename parent::range_type get_range(const T &arg) {
		return parent::get_range(arg.get_head());
	}
};


#if GNUPLOT_ENABLE_CXX11

template <typename Tuple, size_t idx>
struct StdTupUnwinder {
	typedef std::pair<
		typename StdTupUnwinder<Tuple, idx-1>::type,
		typename std::tuple_element<idx, Tuple>::type
	> type;

	static typename ArrayTraits<type>::range_type get_range(const Tuple &arg) {
		return typename ArrayTraits<type>::range_type(
			StdTupUnwinder<Tuple, idx-1>::get_range(arg),
			ArrayTraits<typename std::tuple_element<idx, Tuple>::type>::get_range(std::get<idx>(arg))
		);
	}
};

template <typename Tuple>
struct StdTupUnwinder<Tuple, 0> {
	typedef typename std::tuple_element<0, Tuple>::type type;

	static typename ArrayTraits<type>::range_type get_range(const Tuple &arg) {
		return ArrayTraits<type>::get_range(std::get<0>(arg));
	}
};

template <typename... Args>
class ArrayTraits<std::tuple<Args...> > :
	public ArrayTraits<typename StdTupUnwinder<std::tuple<Args...>, sizeof...(Args)-1>::type>
{
	typedef std::tuple<Args...> Tuple;
	typedef ArrayTraits<typename StdTupUnwinder<Tuple, sizeof...(Args)-1>::type> parent;

public:
	static typename parent::range_type get_range(const Tuple &arg) {
		return StdTupUnwinder<std::tuple<Args...>, sizeof...(Args)-1>::get_range(arg);
	}
};

#endif // GNUPLOT_ENABLE_CXX11


template <typename RT>
class VecOfRange {
	template <typename T, typename PrintMode>
	friend void deref_and_print(std::ostream &, const VecOfRange<T> &, PrintMode);

public:
	VecOfRange() { }
	explicit VecOfRange(const std::vector<RT> &_rvec) : rvec(_rvec) { }

	static const bool is_container = RT::is_container;
	// Don't allow colwrap since it's already wrapped.
	static const bool allow_auto_unwrap = false;

	typedef std::vector<typename RT::value_type> value_type;
	typedef VecOfRange<typename RT::subiter_type> subiter_type;

	bool is_end() const {
		if(rvec.empty()) return true;
		bool ret = rvec[0].is_end();
		for(size_t i=1; i<rvec.size(); i++) {
			if(ret != rvec[i].is_end()) {
				throw std::length_error("columns were different lengths");
			}
		}
		return ret;
	}

	void inc() {
		for(size_t i=0; i<rvec.size(); i++) {
			rvec[i].inc();
		}
	}

	value_type deref() const {
		value_type ret(rvec.size());
		for(size_t i=0; i<rvec.size(); i++) {
			ret[i] = rvec[i].deref();
		}
		return ret;
	}

	subiter_type deref_subiter() const {
		std::vector<typename RT::subiter_type> subvec(rvec.size());
		for(size_t i=0; i<rvec.size(); i++) {
			subvec[i] = rvec[i].deref_subiter();
		}
		return subiter_type(subvec);
	}

private:
	std::vector<RT> rvec;
};

template <typename T>
VecOfRange<typename ArrayTraits<T>::range_type::subiter_type>
get_columns_range(const T &arg) {
	typedef typename ArrayTraits<T>::range_type::subiter_type U;
	std::vector<U> rvec;
	typename ArrayTraits<T>::range_type outer = ArrayTraits<T>::get_range(arg);
	while(!outer.is_end()) {
		rvec.push_back(outer.deref_subiter());
		outer.inc();
	}
	VecOfRange<U> ret(rvec);
	return ret;
}

static bool debug_array_print = 0;

struct ModeText   { static const bool is_text = 1; static const bool is_binfmt = 0; static const bool is_size = 0; };
struct ModeBinary { static const bool is_text = 0; static const bool is_binfmt = 0; static const bool is_size = 0; };
struct ModeBinfmt { static const bool is_text = 0; static const bool is_binfmt = 1; static const bool is_size = 0; };
struct ModeSize   { static const bool is_text = 0; static const bool is_binfmt = 0; static const bool is_size = 1; };

struct ColUnwrapNo  { };
struct ColUnwrapYes { };

struct Mode1D       { static std::string class_name() { return "Mode1D"      ; } };
struct Mode2D       { static std::string class_name() { return "Mode2D"      ; } };
struct Mode1DUnwrap { static std::string class_name() { return "Mode1DUnwrap"; } };
struct Mode2DUnwrap { static std::string class_name() { return "Mode2DUnwrap"; } };
// Support for the legacy behavior that guesses which of the above four modes should be used.
struct ModeAuto     { static std::string class_name() { return "ModeAuto"    ; } };


template <typename T, typename Enable=void>
struct ModeAutoDecoder { };

template <typename T>
struct ModeAutoDecoder<T,
	typename boost::enable_if_c<
		(ArrayTraits<T>::depth == 1)
	>::type>
{
	typedef Mode1D mode;
};

template <typename T>
struct ModeAutoDecoder<T,
	typename boost::enable_if_c<
		(ArrayTraits<T>::depth == 2) &&
		!ArrayTraits<T>::allow_auto_unwrap
	>::type>
{
	typedef Mode2D mode;
};

template <typename T>
struct ModeAutoDecoder<T,
	typename boost::enable_if_c<
		(ArrayTraits<T>::depth == 2) &&
		ArrayTraits<T>::allow_auto_unwrap
	>::type>
{
	typedef Mode1DUnwrap mode;
};

template <typename T>
struct ModeAutoDecoder<T,
	typename boost::enable_if_c<
		(ArrayTraits<T>::depth > 2) &&
		ArrayTraits<T>::allow_auto_unwrap
	>::type>
{
	typedef Mode2DUnwrap mode;
};

template <typename T>
struct ModeAutoDecoder<T,
	typename boost::enable_if_c<
		(ArrayTraits<T>::depth > 2) &&
		!ArrayTraits<T>::allow_auto_unwrap
	>::type>
{
	typedef Mode2D mode;
};


template <typename T>
void send_scalar(std::ostream &stream, const T &arg, ModeText) {
	TextSender<T>::send(stream, arg);
}

template <typename T>
void send_scalar(std::ostream &stream, const T &arg, ModeBinary) {
	BinarySender<T>::send(stream, arg);
}

template <typename T>
void send_scalar(std::ostream &stream, const T &, ModeBinfmt) {
	BinfmtSender<T>::send(stream);
}


template <typename T, typename PrintMode>
typename boost::disable_if_c<T::is_container>::type
deref_and_print(std::ostream &stream, const T &arg, PrintMode) {
	const typename T::value_type &v = arg.deref();
	send_scalar(stream, v, PrintMode());
}

template <typename T, typename PrintMode>
typename boost::enable_if_c<T::is_container>::type
deref_and_print(std::ostream &stream, const T &arg, PrintMode) {
	if(debug_array_print && PrintMode::is_text) stream << "{";
	typename T::subiter_type subrange = arg.deref_subiter();
	bool first = true;
	while(!subrange.is_end()) {
		if(!first && PrintMode::is_text) stream << " ";
		first = false;
		deref_and_print(stream, subrange, PrintMode());
		subrange.inc();
	}
	if(debug_array_print && PrintMode::is_text) stream << "}";
}

// PairOfRange is treated as columns.  In text mode, put a space between columns.
template <typename T, typename U, typename PrintMode>
void deref_and_print(std::ostream &stream, const PairOfRange<T, U> &arg, PrintMode) {
	deref_and_print(stream, arg.l, PrintMode());
	if(PrintMode::is_text) stream << " ";
	deref_and_print(stream, arg.r, PrintMode());
}

// VecOfRange is treated as columns.  In text mode, put a space between columns.
template <typename T, typename PrintMode>
void deref_and_print(std::ostream &stream, const VecOfRange<T> &arg, PrintMode) {
	for(size_t i=0; i<arg.rvec.size(); i++) {
		if(i && PrintMode::is_text) stream << " ";
		deref_and_print(stream, arg.rvec[i], PrintMode());
	}
}

template <size_t Depth, typename T, typename PrintMode>
typename boost::enable_if_c<(Depth==1) && !PrintMode::is_size>::type
print_block(std::ostream &stream, T &arg, PrintMode) {
	for(; !arg.is_end(); arg.inc()) {
		//print_entry(arg.deref());
		deref_and_print(stream, arg, PrintMode());
		// If asked to print the binary format string, only the first element needs to be
		// looked at.
		if(PrintMode::is_binfmt) break;
		if(PrintMode::is_text) stream << std::endl;
	}
}

template <size_t Depth, typename T, typename PrintMode>
typename boost::enable_if_c<(Depth>1) && !PrintMode::is_size>::type
print_block(std::ostream &stream, T &arg, PrintMode) {
	bool first = true;
	for(; !arg.is_end(); arg.inc()) {
		if(first) {
			first = false;
		} else {
			if(PrintMode::is_text) stream << std::endl;
		}
		if(debug_array_print && PrintMode::is_text) stream << "<block>" << std::endl;
		typename T::subiter_type sub = arg.deref_subiter();
		print_block<Depth-1>(stream, sub, PrintMode());
		// If asked to print the binary format string, only the first element needs to be
		// looked at.
		if(PrintMode::is_binfmt) break;
	}
}

// Determine how many elements are in the given range.  Used in the functions below.
template <typename T>
size_t get_range_size(const T &arg) {
	// FIXME - not the fastest way.  Implement a size() method for range.
	size_t ret = 0;
	for(T i=arg; !i.is_end(); i.inc()) ++ret;
	return ret;
}

// Depth==1 and we are asked to print the size of the array.
template <size_t Depth, typename T, typename PrintMode>
typename boost::enable_if_c<(Depth==1) && PrintMode::is_size>::type
print_block(std::ostream &stream, T &arg, PrintMode) {
	stream << get_range_size(arg);
}

// Depth>1 and we are asked to print the size of the array.
template <size_t Depth, typename T, typename PrintMode>
typename boost::enable_if_c<(Depth>1) && PrintMode::is_size>::type
print_block(std::ostream &stream, T &arg, PrintMode) {
	// It seems that size for two dimensional arrays needs the fastest varying index first,
	// contrary to intuition.  The gnuplot documentation is not too clear on this point.
	typename T::subiter_type sub = arg.deref_subiter();
	print_block<Depth-1>(stream, sub, PrintMode());
	stream << "," << get_range_size(arg);
}


template <size_t Depth, typename T, typename PrintMode>
void handle_colunwrap_tag(std::ostream &stream, const T &arg, ColUnwrapNo, PrintMode) {
	GNUPLOT_STATIC_ASSERT_MSG(ArrayTraits<T>::depth >= Depth, "container not deep enough");
	typename ArrayTraits<T>::range_type range = ArrayTraits<T>::get_range(arg);
	print_block<Depth>(stream, range, PrintMode());
}

template <size_t Depth, typename T, typename PrintMode>
void handle_colunwrap_tag(std::ostream &stream, const T &arg, ColUnwrapYes, PrintMode) {
	GNUPLOT_STATIC_ASSERT_MSG(ArrayTraits<T>::depth >= Depth+1, "container not deep enough");
	VecOfRange<typename ArrayTraits<T>::range_type::subiter_type> cols = get_columns_range(arg);
	print_block<Depth>(stream, cols, PrintMode());
}


template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, Mode1D, PrintMode) {
	handle_colunwrap_tag<1>(stream, arg, ColUnwrapNo(), PrintMode());
}

template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, Mode2D, PrintMode) {
	handle_colunwrap_tag<2>(stream, arg, ColUnwrapNo(), PrintMode());
}

template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, Mode1DUnwrap, PrintMode) {
	handle_colunwrap_tag<1>(stream, arg, ColUnwrapYes(), PrintMode());
}

template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, Mode2DUnwrap, PrintMode) {
	handle_colunwrap_tag<2>(stream, arg, ColUnwrapYes(), PrintMode());
}

template <typename T, typename PrintMode>
void handle_organization_tag(std::ostream &stream, const T &arg, ModeAuto, PrintMode) {
	handle_organization_tag(stream, arg, typename ModeAutoDecoder<T>::mode(), PrintMode());
}

template <typename T, typename OrganizationMode, typename PrintMode>
void top_level_array_sender(std::ostream &stream, const T &arg, OrganizationMode, PrintMode) {
	handle_organization_tag(stream, arg, OrganizationMode(), PrintMode());
}

struct FileHandleWrapper {
	FileHandleWrapper(std::FILE *_fh, bool _should_use_pclose) :
		wrapped_fh(_fh), should_use_pclose(_should_use_pclose) { }

	void fh_close() {
		if(should_use_pclose) {
			if(GNUPLOT_PCLOSE(wrapped_fh)) {
				std::cerr << "pclose returned error" << std::endl;
			}
		} else {
			if(fclose(wrapped_fh)) {
				std::cerr << "fclose returned error" << std::endl;
			}
		}
	}

	int fh_fileno() {
		return GNUPLOT_FILENO(wrapped_fh);
	}

	std::FILE *wrapped_fh;
	bool should_use_pclose;
};


class Gnuplot :
	private FileHandleWrapper,
	public boost::iostreams::stream<boost::iostreams::file_descriptor_sink>
{
private:
	static std::string get_default_cmd() {
		GNUPLOT_MSVC_WARNING_4996_PUSH
		char *from_env = std::getenv("GNUPLOT_IOSTREAM_CMD");
		GNUPLOT_MSVC_WARNING_4996_POP
		if(from_env && from_env[0]) {
			return from_env;
		} else {
			return GNUPLOT_DEFAULT_COMMAND;
		}
	}

	static FileHandleWrapper open_cmdline(const std::string &in) {
		std::string cmd = in.empty() ? get_default_cmd() : in;
		assert(!cmd.empty());
		if(cmd[0] == '>') {
			std::string fn = cmd.substr(1);
			GNUPLOT_MSVC_WARNING_4996_PUSH
			FILE *fh = std::fopen(fn.c_str(), "w");
			GNUPLOT_MSVC_WARNING_4996_POP
			if(!fh) throw(std::ios_base::failure("cannot open file "+fn));
			return FileHandleWrapper(fh, false);
		} else {
			FILE *fh = GNUPLOT_POPEN(cmd.c_str(), "w");
			if(!fh) throw(std::ios_base::failure("cannot open pipe "+cmd));
			return FileHandleWrapper(fh, true);
		}
	}

public:
	explicit Gnuplot(const std::string &_cmd="") :
		FileHandleWrapper(open_cmdline(_cmd)),
		boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
			fh_fileno(),
#if BOOST_VERSION >= 104400
			boost::iostreams::never_close_handle
#else
			false
#endif
		),
		feedback(NULL),
		tmp_files(),
		debug_messages(false)
	{
		*this << std::scientific << std::setprecision(18);  // refer <iomanip>
	}

	explicit Gnuplot(FILE *_fh) :
		FileHandleWrapper(_fh, 0),
		boost::iostreams::stream<boost::iostreams::file_descriptor_sink>(
			fh_fileno(),
#if BOOST_VERSION >= 104400
			boost::iostreams::never_close_handle
#else
			false
#endif
		),
		feedback(NULL),
		tmp_files(),
		debug_messages(false)
	{
		*this << std::scientific << std::setprecision(18);  // refer <iomanip>
	}

private:
	// noncopyable
	Gnuplot(const Gnuplot &);
	const Gnuplot& operator=(const Gnuplot &);

public:
	~Gnuplot() {
		if(debug_messages) {
			std::cerr << "ending gnuplot session" << std::endl;
		}

		do_flush();

		fh_close();

		delete feedback;
	}

	void clearTmpfiles() {
		tmp_files.clear();
	}

private:
	void do_flush() {
		*this << std::flush;
		fflush(wrapped_fh);
	}

	std::string make_tmpfile() {
#ifdef GNUPLOT_USE_TMPFILE
		boost::shared_ptr<GnuplotTmpfile> tmp_file(new GnuplotTmpfile());
		tmp_files.push_back(tmp_file);
		return tmp_file->file.string();
#else
		throw(std::logic_error("no filename given and temporary files not enabled"));
#endif // GNUPLOT_USE_TMPFILE
	}

public:

	template <typename T, typename OrganizationMode>
	Gnuplot &send(const T &arg, OrganizationMode) {
		top_level_array_sender(*this, arg, OrganizationMode(), ModeText());
		*this << "e" << std::endl; // gnuplot's "end of array" token
		do_flush(); // probably not really needed, but doesn't hurt
		return *this;
	}

	template <typename T, typename OrganizationMode>
	Gnuplot &sendBinary(const T &arg, OrganizationMode) {
		top_level_array_sender(*this, arg, OrganizationMode(), ModeBinary());
		do_flush(); // probably not really needed, but doesn't hurt
		return *this;
	}

	template <typename T, typename OrganizationMode>
	std::string binfmt(const T &arg, const std::string &arr_or_rec, OrganizationMode) {
		std::ostringstream tmp;
		tmp << " format='";
		top_level_array_sender(tmp, arg, OrganizationMode(), ModeBinfmt());
		assert((arr_or_rec == "array") || (arr_or_rec == "record"));
		tmp << "' " << arr_or_rec << "=(";
		top_level_array_sender(tmp, arg, OrganizationMode(), ModeSize());
		tmp << ")";
		tmp << " ";
		return tmp.str();
	}

	// NOTE: empty filename makes temporary file
	template <typename T, typename OrganizationMode>
	std::string file(const T &arg, std::string filename, OrganizationMode) {
		if(filename.empty()) filename = make_tmpfile();
		std::fstream tmp_stream(filename.c_str(), std::fstream::out);
		top_level_array_sender(tmp_stream, arg, OrganizationMode(), ModeText());
		tmp_stream.close();

		std::ostringstream cmdline;
		// FIXME - hopefully filename doesn't contain quotes or such...
		cmdline << " '" << filename << "' ";
		return cmdline.str();
	}

	// NOTE: empty filename makes temporary file
	template <typename T, typename OrganizationMode>
	std::string binaryFile(const T &arg, std::string filename, const std::string &arr_or_rec, OrganizationMode) {
		if(filename.empty()) filename = make_tmpfile();
		std::fstream tmp_stream(filename.c_str(), std::fstream::out | std::fstream::binary);
		top_level_array_sender(tmp_stream, arg, OrganizationMode(), ModeBinary());
		tmp_stream.close();

		std::ostringstream cmdline;
		// FIXME - hopefully filename doesn't contain quotes or such...
		cmdline << " '" << filename << "' binary" << binfmt(arg, arr_or_rec, OrganizationMode());
		return cmdline.str();
	}


	template <typename T> Gnuplot GNUPLOT_DEPRECATE("use send1d or send2d")
		&send(const T &arg) { return send(arg, ModeAuto()); }

	template <typename T> std::string GNUPLOT_DEPRECATE("use binfmt1d or binfmt2d")
		binfmt(const T &arg, const std::string &arr_or_rec="array")
		{ return binfmt(arg, arr_or_rec,  ModeAuto()); }

	template <typename T> Gnuplot GNUPLOT_DEPRECATE("use sendBinary1d or sendBinary2d")
		&sendBinary(const T &arg) { return sendBinary(arg, ModeAuto()); }

	template <typename T> std::string GNUPLOT_DEPRECATE("use file1d or file2d")
		file(const T &arg, const std::string &filename="")
		{ return file(arg, filename, ModeAuto()); }

	template <typename T> std::string GNUPLOT_DEPRECATE("use binArr1d or binArr2d")
		binaryFile(const T &arg, const std::string &filename="", const std::string &arr_or_rec="array")
		{ return binaryFile(arg, filename, arr_or_rec,  ModeAuto()); }


	template <typename T> Gnuplot &send1d         (const T &arg) { return send(arg, Mode1D      ()); }
	template <typename T> Gnuplot &send2d         (const T &arg) { return send(arg, Mode2D      ()); }
	template <typename T> Gnuplot &send1d_colmajor(const T &arg) { return send(arg, Mode1DUnwrap()); }
	template <typename T> Gnuplot &send2d_colmajor(const T &arg) { return send(arg, Mode2DUnwrap()); }

	template <typename T> Gnuplot &sendBinary1d         (const T &arg) { return sendBinary(arg, Mode1D      ()); }
	template <typename T> Gnuplot &sendBinary2d         (const T &arg) { return sendBinary(arg, Mode2D      ()); }
	template <typename T> Gnuplot &sendBinary1d_colmajor(const T &arg) { return sendBinary(arg, Mode1DUnwrap()); }
	template <typename T> Gnuplot &sendBinary2d_colmajor(const T &arg) { return sendBinary(arg, Mode2DUnwrap()); }

	template <typename T> std::string file1d         (const T &arg, const std::string &filename="") { return file(arg, filename, Mode1D      ()); }
	template <typename T> std::string file2d         (const T &arg, const std::string &filename="") { return file(arg, filename, Mode2D      ()); }
	template <typename T> std::string file1d_colmajor(const T &arg, const std::string &filename="") { return file(arg, filename, Mode1DUnwrap()); }
	template <typename T> std::string file2d_colmajor(const T &arg, const std::string &filename="") { return file(arg, filename, Mode2DUnwrap()); }

	template <typename T> std::string binFmt1d         (const T &arg, const std::string &arr_or_rec) { return binfmt(arg, arr_or_rec,  Mode1D      ()); }
	template <typename T> std::string binFmt2d         (const T &arg, const std::string &arr_or_rec) { return binfmt(arg, arr_or_rec,  Mode2D      ()); }
	template <typename T> std::string binFmt1d_colmajor(const T &arg, const std::string &arr_or_rec) { return binfmt(arg, arr_or_rec,  Mode1DUnwrap()); }
	template <typename T> std::string binFmt2d_colmajor(const T &arg, const std::string &arr_or_rec) { return binfmt(arg, arr_or_rec,  Mode2DUnwrap()); }

	template <typename T> std::string binFile1d         (const T &arg, const std::string &arr_or_rec, const std::string &filename="") { return binaryFile(arg, filename, arr_or_rec,  Mode1D      ()); }
	template <typename T> std::string binFile2d         (const T &arg, const std::string &arr_or_rec, const std::string &filename="") { return binaryFile(arg, filename, arr_or_rec,  Mode2D      ()); }
	template <typename T> std::string binFile1d_colmajor(const T &arg, const std::string &arr_or_rec, const std::string &filename="") { return binaryFile(arg, filename, arr_or_rec,  Mode1DUnwrap()); }
	template <typename T> std::string binFile2d_colmajor(const T &arg, const std::string &arr_or_rec, const std::string &filename="") { return binaryFile(arg, filename, arr_or_rec,  Mode2DUnwrap()); }


#ifdef GNUPLOT_ENABLE_FEEDBACK
public:
	// Input variables are set to the mouse position and button.  If the gnuplot
	// window is closed, button -1 is returned.  The msg parameter is the prompt
	// that is printed to the console.
	void getMouse(
		double &mx, double &my, int &mb,
		std::string msg="Click Mouse!"
	) {
		allocFeedback();

		*this << "set mouse" << std::endl;
		*this << "pause mouse \"" << msg << "\\n\"" << std::endl;
		*this << "if (exists(\"MOUSE_X\")) print MOUSE_X, MOUSE_Y, MOUSE_BUTTON; else print 0, 0, -1;" << std::endl;
		if(debug_messages) {
			std::cerr << "begin scanf" << std::endl;
		}
		if(3 != fscanf(feedback->handle(), "%50lf %50lf %50d", &mx, &my, &mb)) {
			throw std::runtime_error("could not parse reply");
		}
		if(debug_messages) {
			std::cerr << "end scanf" << std::endl;
		}
	}

private:
	void allocFeedback() {
		if(!feedback) {
#ifdef GNUPLOT_ENABLE_PTY
			feedback = new GnuplotFeedbackPty(debug_messages);
//#elif defined GNUPLOT_USE_TMPFILE
//// Currently this doesn't work since fscanf doesn't block (need something like "tail -f")
//			feedback = new GnuplotFeedbackTmpfile(debug_messages);
#else
			// This shouldn't happen because we are in an `#ifdef GNUPLOT_ENABLE_FEEDBACK`
			// block which should only be activated if GNUPLOT_ENABLE_PTY is defined.
			GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "No feedback mechanism defined.");
#endif
			*this << "set print \"" << feedback->filename() << "\"" << std::endl;
		}
	}
#endif // GNUPLOT_ENABLE_FEEDBACK

private:
	GnuplotFeedback *feedback;
#ifdef GNUPLOT_USE_TMPFILE
	std::vector<boost::shared_ptr<GnuplotTmpfile> > tmp_files;
#else
	// just a placeholder
	std::vector<int> tmp_files;
#endif // GNUPLOT_USE_TMPFILE

public:
	bool debug_messages;
};


} // namespace gnuplotio

using gnuplotio::Gnuplot;

#endif // GNUPLOT_IOSTREAM_H


#ifdef BZ_BLITZ_H
#ifndef GNUPLOT_BLITZ_SUPPORT_LOADED
#define GNUPLOT_BLITZ_SUPPORT_LOADED
namespace gnuplotio {

template <typename T, int N>
struct BinfmtSender<blitz::TinyVector<T, N> > {
	static void send(std::ostream &stream) {
		for(int i=0; i<N; i++) {
			BinfmtSender<T>::send(stream);
		}
	}
};

template <typename T, int N>
struct TextSender<blitz::TinyVector<T, N> > {
	static void send(std::ostream &stream, const blitz::TinyVector<T, N> &v) {
		for(int i=0; i<N; i++) {
			if(i) stream << " ";
			TextSender<T>::send(stream, v[i]);
		}
	}
};

template <typename T, int N>
struct BinarySender<blitz::TinyVector<T, N> > {
	static void send(std::ostream &stream, const blitz::TinyVector<T, N> &v) {
		for(int i=0; i<N; i++) {
			BinarySender<T>::send(stream, v[i]);
		}
	}
};

class Error_WasBlitzPartialSlice { };

template <typename T, int ArrayDim, int SliceDim>
class BlitzIterator {
public:
	BlitzIterator() : p(NULL) { }
	BlitzIterator(
		const blitz::Array<T, ArrayDim> *_p,
		const blitz::TinyVector<int, ArrayDim> _idx
	) : p(_p), idx(_idx) { }

	typedef Error_WasBlitzPartialSlice value_type;
	typedef BlitzIterator<T, ArrayDim, SliceDim-1> subiter_type;
	static const bool is_container = true;

	// FIXME - it would be nice to also handle one-based arrays
	bool is_end() const {
		return idx[ArrayDim-SliceDim] == p->shape()[ArrayDim-SliceDim];
	}

	void inc() {
		++idx[ArrayDim-SliceDim];
	}

	value_type deref() const {
		GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "cannot deref a blitz slice");
		throw std::logic_error("static assert should have been triggered by this point");
	}

	subiter_type deref_subiter() const {
		return BlitzIterator<T, ArrayDim, SliceDim-1>(p, idx);
	}

private:
	const blitz::Array<T, ArrayDim> *p;
	blitz::TinyVector<int, ArrayDim> idx;
};

template <typename T, int ArrayDim>
class BlitzIterator<T, ArrayDim, 1> {
public:
	BlitzIterator() : p(NULL) { }
	BlitzIterator(
		const blitz::Array<T, ArrayDim> *_p,
		const blitz::TinyVector<int, ArrayDim> _idx
	) : p(_p), idx(_idx) { }

	typedef T value_type;
	typedef Error_WasNotContainer subiter_type;
	static const bool is_container = false;

	// FIXME - it would be nice to also handle one-based arrays
	bool is_end() const {
		return idx[ArrayDim-1] == p->shape()[ArrayDim-1];
	}

	void inc() {
		++idx[ArrayDim-1];
	}

	value_type deref() const {
		return (*p)(idx);
	}

	subiter_type deref_subiter() const {
		GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "argument was not a container");
		throw std::logic_error("static assert should have been triggered by this point");
	}

private:
	const blitz::Array<T, ArrayDim> *p;
	blitz::TinyVector<int, ArrayDim> idx;
};

template <typename T, int ArrayDim>
class ArrayTraits<blitz::Array<T, ArrayDim> > : public ArrayTraitsDefaults<T> {
public:
	static const bool allow_auto_unwrap = false;
	static const size_t depth = ArrayTraits<T>::depth + ArrayDim;

	typedef BlitzIterator<T, ArrayDim, ArrayDim> range_type;

	static range_type get_range(const blitz::Array<T, ArrayDim> &arg) {
		blitz::TinyVector<int, ArrayDim> start_idx;
		start_idx = 0;
		return range_type(&arg, start_idx);
	}
};

} // namespace gnuplotio
#endif // GNUPLOT_BLITZ_SUPPORT_LOADED
#endif // BZ_BLITZ_H


#ifdef ARMA_INCLUDES
#ifndef GNUPLOT_ARMADILLO_SUPPORT_LOADED
#define GNUPLOT_ARMADILLO_SUPPORT_LOADED
namespace gnuplotio {

template <typename T> struct dont_treat_as_stl_container<arma::Row  <T> > { typedef boost::mpl::bool_<true> type; };
template <typename T> struct dont_treat_as_stl_container<arma::Col  <T> > { typedef boost::mpl::bool_<true> type; };
template <typename T> struct dont_treat_as_stl_container<arma::Mat  <T> > { typedef boost::mpl::bool_<true> type; };
template <typename T> struct dont_treat_as_stl_container<arma::Cube <T> > { typedef boost::mpl::bool_<true> type; };
template <typename T> struct dont_treat_as_stl_container<arma::field<T> > { typedef boost::mpl::bool_<true> type; };

// {{{3 Cube

template <typename T>
class ArrayTraits<arma::Cube<T> > : public ArrayTraitsDefaults<T> {
	class SliceRange {
	public:
		SliceRange() : p(NULL), col(0), slice(0) { }
		explicit SliceRange(const arma::Cube<T> *_p, size_t _row, size_t _col) :
			p(_p), row(_row), col(_col), slice(0) { }

		typedef T value_type;
		typedef Error_WasNotContainer subiter_type;
		static const bool is_container = false;

		bool is_end() const { return slice == p->n_slices; }

		void inc() { ++slice; }

		value_type deref() const {
			return (*p)(row, col, slice);
		}

		subiter_type deref_subiter() const {
			GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "argument was not a container");
			throw std::logic_error("static assert should have been triggered by this point");
		}

	private:
		const arma::Cube<T> *p;
		size_t row, col, slice;
	};

	class ColRange {
	public:
		ColRange() : p(NULL), row(0), col(0) { }
		explicit ColRange(const arma::Cube<T> *_p, size_t _row) :
			p(_p), row(_row), col(0) { }

		typedef T value_type;
		typedef SliceRange subiter_type;
		static const bool is_container = true;

		bool is_end() const { return col == p->n_cols; }

		void inc() { ++col; }

		value_type deref() const {
			GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "can't call deref on an armadillo cube col");
			throw std::logic_error("static assert should have been triggered by this point");
		}

		subiter_type deref_subiter() const {
			return subiter_type(p, row, col);
		}

	private:
		const arma::Cube<T> *p;
		size_t row, col;
	};

	class RowRange {
	public:
		RowRange() : p(NULL), row(0) { }
		explicit RowRange(const arma::Cube<T> *_p) : p(_p), row(0) { }

		typedef T value_type;
		typedef ColRange subiter_type;
		static const bool is_container = true;

		bool is_end() const { return row == p->n_rows; }

		void inc() { ++row; }

		value_type deref() const {
			GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "can't call deref on an armadillo cube row");
			throw std::logic_error("static assert should have been triggered by this point");
		}

		subiter_type deref_subiter() const {
			return subiter_type(p, row);
		}

	private:
		const arma::Cube<T> *p;
		size_t row;
	};

public:
	static const bool allow_auto_unwrap = false;
	static const size_t depth = ArrayTraits<T>::depth + 3;

	typedef RowRange range_type;

	static range_type get_range(const arma::Cube<T> &arg) {
		//std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
		return range_type(&arg);
	}
};



template <typename RF, typename T>
class ArrayTraits_ArmaMatOrField : public ArrayTraitsDefaults<T> {
	class ColRange {
	public:
		ColRange() : p(NULL), row(0), col(0) { }
		explicit ColRange(const RF *_p, size_t _row) :
			p(_p), row(_row), col(0) { }

		typedef T value_type;
		typedef Error_WasNotContainer subiter_type;
		static const bool is_container = false;

		bool is_end() const { return col == p->n_cols; }

		void inc() { ++col; }

		value_type deref() const {
			return (*p)(row, col);
		}

		subiter_type deref_subiter() const {
			GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "argument was not a container");
			throw std::logic_error("static assert should have been triggered by this point");
		}

	private:
		const RF *p;
		size_t row, col;
	};

	class RowRange {
	public:
		RowRange() : p(NULL), row(0) { }
		explicit RowRange(const RF *_p) : p(_p), row(0) { }

		typedef T value_type;
		typedef ColRange subiter_type;
		static const bool is_container = true;

		bool is_end() const { return row == p->n_rows; }

		void inc() { ++row; }

		value_type deref() const {
			GNUPLOT_STATIC_ASSERT_MSG((sizeof(T) == 0), "can't call deref on an armadillo matrix row");
			throw std::logic_error("static assert should have been triggered by this point");
		}

		subiter_type deref_subiter() const {
			return subiter_type(p, row);
		}

	private:
		const RF *p;
		size_t row;
	};

public:
	static const bool allow_auto_unwrap = false;
	static const size_t depth = ArrayTraits<T>::depth + 2;

	typedef RowRange range_type;

	static range_type get_range(const RF &arg) {
		return range_type(&arg);
	}
};

template <typename T>
class ArrayTraits<arma::field<T> > : public ArrayTraits_ArmaMatOrField<arma::field<T>, T> { };

template <typename T>
class ArrayTraits<arma::Mat<T> > : public ArrayTraits_ArmaMatOrField<arma::Mat<T>, T> { };


template <typename T>
class ArrayTraits<arma::Row<T> > : public ArrayTraitsDefaults<T> {
public:
	static const bool allow_auto_unwrap = false;

	typedef IteratorRange<typename arma::Row<T>::const_iterator, T> range_type;

	static range_type get_range(const arma::Row<T> &arg) {
		//std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
		return range_type(arg.begin(), arg.end());
	}
};


template <typename T>
class ArrayTraits<arma::Col<T> > : public ArrayTraitsDefaults<T> {
public:
	static const bool allow_auto_unwrap = false;

	typedef IteratorRange<typename arma::Col<T>::const_iterator, T> range_type;

	static range_type get_range(const arma::Col<T> &arg) {
		//std::cout << arg.n_elem << "," << arg.n_rows << "," << arg.n_cols << std::endl;
		return range_type(arg.begin(), arg.end());
	}
};


} // namespace gnuplotio
#endif // GNUPLOT_ARMADILLO_SUPPORT_LOADED
#endif // ARMA_INCLUDES

