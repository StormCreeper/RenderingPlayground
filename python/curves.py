import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

def g(x, mu, t1, t2):
    return np.exp(-(np.where(x < mu, t1, t2) * (x - mu)) ** 2 / 2)

# From wikipedia
def XYZsensitivity(lambda_):
    lambda_nm = lambda_ * 1e9
    x = 1.056 * g(lambda_nm, 599.8, 0.0264, 0.0323) + 0.362 * g(lambda_nm, 442, 0.0624, 0.0374) + -0.065 * g(lambda_nm, 501.1, 0.0490, 0.0382)
    y = 0.821 * g(lambda_nm, 568.8, 0.0213, 0.0247) + 0.286 * g(lambda_nm, 530.9, 0.0613, 0.0322)
    z = 1.217 * g(lambda_nm, 437, 0.0845, 0.0278) + 0.681 * g(lambda_nm, 459, 0.0385, 0.0725)
    return x, y, z

def gaussian(x, mu, var, A):
    return A * np.exp(-2 * (x - mu) ** 2  / var / 8.)

def gaussianFit(x, y):  
    mu = np.sum(x * y) / np.sum(y)
    var = (np.abs(np.sum(y * (x - mu) ** 2) / np.sum(y))) / 4
    A = np.max(y)
    
    popt, _ = curve_fit(gaussian, x, y, p0=[mu, var, A])
    
    return popt

def gaussianTF(k, mu, var, A):
    phase = k  * 2 * np.pi
    return A * np.sqrt(2 * np.pi * var) * np.exp(-1j * phase * mu) * np.exp(- var * phase ** 2)

def plot_xyz(wavelengths, X, Y, Z, title, xlabel):
    plt.figure(figsize=(10, 5))
    plt.plot(wavelengths, X, label='X', color='red')
    plt.plot(wavelengths, Y, label='Y', color='green')
    plt.plot(wavelengths, Z, label='Z', color='blue')

    plt.xlabel(xlabel)
    plt.ylabel('Values')
    plt.title(title)
    plt.legend()
    plt.grid(True)

def Sj(sj, nu):
    return sj / (nu ** 2)

if __name__ == "__main__":

    lambda_ = np.arange(360, 830, 1) * 1e-9

    (X, Y, Z) = XYZsensitivity(lambda_)
    
    plot_xyz(lambda_, X, Y, Z, 'XYZ Sensitivity', 'Wavelength (m)')
    
    nu = 1 / lambda_  # Convert wavelength to frequency domain
    
    # evenly spaced frequency domain
    nu = np.linspace(nu[0], nu[-1], len(nu))

    lambda_ = 1 / nu
    # Resample the sensitivity functions
    (X, Y, Z) = XYZsensitivity(lambda_)
    
    X_nu = Sj(X, nu)
    Y_nu = Sj(Y, nu)
    Z_nu = Sj(Z, nu)

    plot_xyz(nu, X_nu, Y_nu, Z_nu, 'XYZ Sensitivity (Frequency)', 'Spatial Frequency (m^-1)')
    
    # fit
    X_fit = gaussianFit(nu, X_nu)
    Y_fit = gaussianFit(nu, Y_nu)
    Z_fit = gaussianFit(nu, Z_nu)
    X2_fit = gaussianFit(nu, X_nu - gaussian(nu, *X_fit))

    print(f"vec3 mu = vec3({X_fit[0]}, {Y_fit[0]}, {Z_fit[0]});")
    print(f"vec3 var = vec3({X_fit[1]}, {Y_fit[1]}, {Z_fit[1]});")
    print(f"vec3 A = vec3({X_fit[2]}, {Y_fit[2]}, {Z_fit[2]});")
    print(f"vec3 X_2 = vec3({X2_fit[0]}, {X2_fit[1]}, {X2_fit[2]});")

    plt.plot(nu, gaussian(nu, *X_fit) + gaussian(nu, *X2_fit), label='X Fit', color='red', linestyle='--')
    plt.plot(nu, gaussian(nu, *Y_fit), label='Y Fit', color='green', linestyle='--')
    plt.plot(nu, gaussian(nu, *Z_fit), label='Z Fit', color='blue', linestyle='--')
    
    plt.legend()

    res = 10

    X_fft = np.fft.fft(X_nu, n=len(nu) * res)
    Y_fft = np.fft.fft(Y_nu, n=len(nu) * res)
    Z_fft = np.fft.fft(Z_nu, n=len(nu) * res)

    X_fft = X_fft / np.max(np.abs(X_fft))
    Y_fft = Y_fft / np.max(np.abs(Y_fft))
    Z_fft = Z_fft / np.max(np.abs(Z_fft))

    # Compute the frequency domain
    freq = np.fft.fftfreq(len(nu) * res, d=nu[1] - nu[0])

    # X_fft_fit = np.fft.fft(gaussian(nu, *X_fit), n=len(nu) * res) + np.fft.fft(gaussian(nu, *X2_fit), n=len(nu) * res)
    # Y_fft_fit = np.fft.fft(gaussian(nu, *Y_fit), n=len(nu) * res)
    # Z_fft_fit = np.fft.fft(gaussian(nu, *Z_fit), n=len(nu) * res)

    X_fft_fit = gaussianTF(freq, *X_fit) + gaussianTF(freq, *X2_fit)
    Y_fft_fit = gaussianTF(freq, *Y_fit)
    Z_fft_fit = gaussianTF(freq, *Z_fit)

    X_fft_fit_0 = np.max(np.abs(X_fft_fit))
    Y_fft_fit_0 = np.max(np.abs(Y_fft_fit))
    Z_fft_fit_0 = np.max(np.abs(Z_fft_fit))

    norm_factor = (X_fft_fit_0 + Y_fft_fit_0 + Z_fft_fit_0) / 3

    print(f"float normFactor = {norm_factor};" )

    X_fft_fit = X_fft_fit / norm_factor
    Y_fft_fit = Y_fft_fit / norm_factor
    Z_fft_fit = Z_fft_fit / norm_factor

    # sort X, Y, Z values by frequency
    X_fft = X_fft[np.argsort(freq)]
    Y_fft = Y_fft[np.argsort(freq)]
    Z_fft = Z_fft[np.argsort(freq)]
    
    X_fft_fit = X_fft_fit[np.argsort(freq)]
    Y_fft_fit = Y_fft_fit[np.argsort(freq)]
    Z_fft_fit = Z_fft_fit[np.argsort(freq)]

    freq = freq[np.argsort(freq)]
    
    plot_xyz(freq, np.abs(X_fft), np.abs(Y_fft), np.abs(Z_fft), 'XYZ Sensitivity (Fourier Domain)', 'm ?')

    plt.plot(freq, np.abs(X_fft_fit), label='X Fit FFT', color='red', linestyle='--')
    plt.plot(freq, np.abs(Y_fft_fit), label='Y Fit FFT', color='green', linestyle='--')
    plt.plot(freq, np.abs(Z_fft_fit), label='Z Fit FFT', color='blue', linestyle='--')
    
    plt.legend()

    plt.show()
